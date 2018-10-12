#include "types.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <OpenImageIO/imageio.h>
#include <cxxopts.hpp>

namespace
{

template <typename T>
void writeImage(string_view _filename, span<T> _data, const uint2 _imageDim)
{
  std::cout<<"Writing image to "<<_filename<<'\n';
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageOutput, void(*)(ImageOutput*)> output(
      ImageOutput::create(_filename.data()),
      [](auto ptr) { ptr->close(); delete ptr; }
      );
  ImageSpec is(_imageDim.x, _imageDim.y, sizeof(T)/sizeof(fpreal), TypeDesc::FLOAT);
  output->open(_filename.data(), is);
  output->write_image(TypeDesc::FLOAT, _data.data());
}

auto readImage(string_view _filename)
{
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageInput, void(*)(ImageInput*)> input(
      ImageInput::open(_filename.data()),
      [](auto ptr) { ptr->close(); delete ptr; }
      );
  // Get the image specification and store the dimensions
  auto&& spec = input->spec();
  uint2 dim {spec.width, spec.height};

  // Allocated an array for our data
  auto data = std::make_unique<fpreal3[]>(dim.x * dim.y);
  // Read the data into our array, with a specified stride of 3 floats (RGB), we ignore the alpha channel
  input->read_image(TypeDesc::FLOAT, data.get(), sizeof(fpreal3), AutoStride, AutoStride);
  
  // return an owning span 
  struct OwningSpan
  {
    std::unique_ptr<fpreal3[]> m_data;
    uint2 m_imageDim;
  };
  return OwningSpan{std::move(data), std::move(dim)};
}

struct Region
{
  uint2 m_startPixel;
  std::unique_ptr<fpreal[]> m_albedoIntensities;

  uint2 getPixelCoordFromLocal(uint2 _coord) const noexcept
  {
    return {m_startPixel.x + _coord.x, m_startPixel.y + _coord.y};
  }

  fpreal* getAlbedoIntensity(uinteger _id, const uint2 _imageDim, const uinteger _regionScale)
  {
    auto x = _id % _imageDim.y - m_startPixel.x;
    auto y = _id / _imageDim.y - m_startPixel.y;
    return &m_albedoIntensities[y * _regionScale + x];
  }

  template <typename F>
  void for_each_pixel(F&& func, const uint2 _imageDim, const uinteger _regionScale) const noexcept
  {
    for (uinteger x = 0u; x < _regionScale; ++x)
    for (uinteger y = 0u; y < _regionScale; ++y)
    {
      uint2 localCoord {x,y};
      auto local = localCoord.y * _regionScale + localCoord.x;
      auto pixelCoord = getPixelCoordFromLocal(localCoord);
      auto pixel = pixelCoord.y * _imageDim.x + pixelCoord.x;
      func(pixel, local);
    }
  }
};


auto generateRegions(const uint2 _imageDim, const uinteger _regionScale, const fpreal* const _albedoIntensities)
{
  // Number of regions in each axis, is equivalent to the size of the image in those axis A,
  // minus the last index within a Region (R-1)
  // N = A - (R - 1) <=> N = A - R + 1
  uint2 numRegions {_imageDim.x - _regionScale +1, _imageDim.y - _regionScale +1};
  // Store the total number of regions in the image
  auto totalNumRegions = numRegions.x * numRegions.y;
  // Number of pixels contained within a region
  auto numPixelsInRegion = _regionScale * _regionScale;
  // Allocate storage for the regions
  auto regions = std::make_unique<Region[]>(totalNumRegions);
  // Pixel regions contains a list of regions per pixel, which contain the pixel
  auto pixelRegions = std::make_unique<std::vector<Region*>[]>(_imageDim.x * _imageDim.y);

  for (uinteger x = 0u; x < numRegions.x; ++x) 
  for (uinteger y = 0u; y < numRegions.y; ++y)
  {
    // Construct our region
    auto&& region = regions[y * numRegions.x + x];
    region =  Region{{x, y}, std::make_unique<fpreal[]>(numPixelsInRegion)};
    // For every pixel in the region, we push a pointer to this region, into their pixelRegion list
    for (uinteger px = x; px < x + _regionScale; ++px)
    for (uinteger py = y; py < y + _regionScale; ++py)
    {
      pixelRegions[py * _imageDim.x + px].push_back(&region);
    }
    // Init the albedo for our new region
    region.for_each_pixel([&](auto pixel, auto local)
        {
          region.m_albedoIntensities[local] = _albedoIntensities[pixel];
        },
        _imageDim, _regionScale
    );
  }
  // return our regions, and pixel regions 
  struct RegionData
  {
    decltype(regions) m_regions;
    decltype(pixelRegions) m_pixelRegions;
    decltype(numRegions) m_dim;
  };
  return RegionData{std::move(regions), std::move(pixelRegions), std::move(numRegions)};
}

template <typename T>
struct remove_cvref 
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename C>
auto average(C&& _elems)
{
  using T = remove_cvref_t<decltype(_elems[0])>;
  auto total = static_cast<T>(0);
  for (auto&& e : _elems) total += e;
  return total / _elems.size();
}

// Define our hashing algorithm as quantization of r and g into slots,
// to give slots^2 possible chroma values
uinteger hashChroma(fpreal3 _chroma, fpreal3 _max, uinteger _slots) noexcept
{
  auto last = _slots - 1;
  uinteger x = (_chroma.x / _max.x) * last;
  uinteger y = (_chroma.y / _max.y) * last;
  return y * last + x;
}

std::unique_ptr<fpreal3[]> calculateChroma(span<const fpreal3> _sourceImage, span<const fpreal> _intensity) 
{
  auto size = _sourceImage.size();
  // {r/i, g/i, 3 - r/i - g/i}
  auto chroma = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) 
  {
    chroma[i].r = _sourceImage[i].r / _intensity[i];
    chroma[i].g = _sourceImage[i].g / _intensity[i];
    chroma[i].b = 3.0f - chroma[i].r - chroma[i].g;
  }
  return chroma;
}

template <typename T>
auto makeSpan(std::unique_ptr<T[]>& _data, uinteger _size)
{
  return span<T>{_data.get(), std::move(_size)};
}

inline static auto getParser()
{
  cxxopts::Options parser("Shading Separator", "Implementation of microsofts Appgen");
  parser.allow_unrecognised_options().add_options()
  ("h,help", "Print help")
  ("s,source", "Source file name", cxxopts::value<std::string>())
  ("i,iterations", "Seperation iterations", cxxopts::value<uinteger>()->default_value("3"))
  ("r,region", "Region scale", cxxopts::value<uinteger>()->default_value("20"))
  ("o,output", "Output file name (no extension)", cxxopts::value<std::string>()->default_value("shading"))
  ("f,format", "Output file format (the extension)", cxxopts::value<std::string>()->default_value("png"))
  ;
  return parser;
}

void clampExtremeties(span<fpreal3> io_image)
{
  //TODO: take caps as input
  // Remove highlights and shadows
  static const fpreal3 shadowCap    (25.f  / 255.f);
  static const fpreal3 highlightCap (235.f / 255.f);
  for (auto&& pixel : io_image)
  {
    pixel = glm::clamp(pixel, shadowCap, highlightCap);
  }
}

std::unique_ptr<fpreal[]> calculateIntensity(span<fpreal3> _image)
{
  auto numPixels = _image.size();
  // Extract the intensity as the average of rgb
  static constexpr fpreal third = 1.0f / 3.0f;
  //TODO: do not default construct
  auto intensity = std::make_unique<fpreal[]>(numPixels);
  for (uinteger i = 0; i < numPixels; ++i) 
  {
    auto&& pixel = _image[i];
    intensity[i] = (pixel.x + pixel.y + pixel.z) * third;
  }
  return intensity;
}

void quantizeChromas(const Region& _region, span<std::vector<uint2>> o_quantizedChromas, span<fpreal3> _chromas, const uinteger _numSlots, const uint2 _imageDimensions, const uinteger _regionScale)
{
  // Group using the chromas, storing the index into the current region that gives us the real pixel
  // We first copy the chroma values for the region into a local array, and simultaneously find the
  // maximum chroma value for quantization
  auto maxChroma = fpreal3(0.f);
  _region.for_each_pixel([&](auto pixel, auto)
      {
        auto chromaVal = _chromas[pixel];
        maxChroma = glm::max(maxChroma, chromaVal);
      },
      _imageDimensions, _regionScale
  );
  
  // Now we hash our copied chroma values
  _region.for_each_pixel([&](auto pixel, auto local)
      {
        auto qChromaHash = hashChroma(_chromas[pixel], maxChroma, _numSlots);
        o_quantizedChromas[qChromaHash].emplace_back(local % _regionScale, local / _regionScale);
      },
      _imageDimensions, _regionScale
  );
}

std::unique_ptr<fpreal[]> calculateCommonChromaIntensitySums(const Region& _region, span<fpreal3> _chroma, span<fpreal> _intensity, const uinteger _numSlots, const uint2 _imageDimensions, const uinteger _regionScale)
{
  static const uinteger numUniqueChromas = _numSlots * _numSlots;
  auto quantizedChromaData = std::make_unique<std::vector<uint2>[]>(numUniqueChromas);
  auto quantizedChromas = makeSpan(quantizedChromaData, numUniqueChromas); 
  quantizeChromas(
      _region, 
      quantizedChromas,
      _chroma,
      _numSlots, 
      _imageDimensions, 
      _regionScale
      );

  auto commonChromaIntensitySums = std::make_unique<fpreal[]>(_regionScale*_regionScale);
  for (auto&& qChroma : quantizedChromas)
  {
    fpreal sum(0.f);
    for (auto&& rpx : qChroma) 
    {
      auto pixelCoord = _region.getPixelCoordFromLocal(rpx);
      sum += _intensity[pixelCoord.y * _imageDimensions.x + pixelCoord.x];
    }
    auto averageChromaIntensity = sum / static_cast<fpreal>(qChroma.size());
    for (auto&& rpx : qChroma) 
    {
      commonChromaIntensitySums[rpx.y * _regionScale + rpx.x] = averageChromaIntensity;
    }
  }
  return commonChromaIntensitySums;
}

void calcExpectedAlbedoIntensity(span<fpreal> io_albedoIntensity, span<std::vector<Region*>> _pixelRegions, const uint2 _imageDimensions, const uinteger _regionScale)
{
  auto numPixels = io_albedoIntensity.size();
  // For each pixel
  for (uinteger pixelId = 0; pixelId < numPixels; ++pixelId)
  {
    // get average of albedo intensity for all regions containing this pixel
    fpreal totalAlbedoIntensity = 0.f;
    for (auto rptr : _pixelRegions[pixelId])
    {
      totalAlbedoIntensity += *rptr->getAlbedoIntensity(pixelId, _imageDimensions, _regionScale);
    }
    // Store the albedo intensity for the current pixel
    io_albedoIntensity[pixelId] = totalAlbedoIntensity/ _pixelRegions[pixelId].size();
    for (auto rptr : _pixelRegions[pixelId])
    {
      // Update the stored albedo intensity for each region
      *rptr->getAlbedoIntensity(pixelId, _imageDimensions, _regionScale) = io_albedoIntensity[pixelId];
    }
  }
}

}

int main(int argc, char* argv[])
{
  auto parser = getParser();
  const auto args = parser.parse(argc, argv);
  if (args.count("help") || !args.count("source"))
  {
    std::cout<<parser.help()<<'\n';
    std::exit(0);
  }

  // Read the source image in as an array of rgbf
  auto imgResult = readImage(args["source"].as<std::string>());
  auto&& sourceImage = imgResult.m_data;
  auto&& imageDimensions = imgResult.m_imageDim;
  auto numPixels = imageDimensions.x * imageDimensions.y;

  clampExtremeties(makeSpan(sourceImage, numPixels));

  auto intensity = calculateIntensity(makeSpan(sourceImage, numPixels));

  // Extract the chroma of the image using our intensity
  auto chroma = calculateChroma(makeSpan(sourceImage, numPixels), makeSpan(intensity, numPixels));
  // Our shading intensity defaults to one, so albedo intensity = source intensity
  // i = si * ai
  // //TODO: Do not default construct
  auto albedoIntensity = std::make_unique<fpreal[]>(numPixels);
  std::memcpy(albedoIntensity.get(), intensity.get(), sizeof(albedoIntensity[0]) * numPixels);

  // Divide our images into regions, 
  // we store the regions using pixel coordinates that represent their top left pixel.
  // We know the width and height is the same for each
  const uinteger regionScale = args["region"].as<uinteger>();
  const uinteger numPixelsInRegion  = regionScale * regionScale;
  auto regionResult = generateRegions(imageDimensions, regionScale, albedoIntensity.get());
  auto&& regions = regionResult.m_regions;
  auto&& pixelRegions = regionResult.m_pixelRegions;
  auto&& numRegionsXY = regionResult.m_dim;
  auto numRegions = numRegionsXY.x * numRegionsXY.y;
  std::cout<<"Regions Complete.\n";

  const auto maxIter = args["iterations"].as<uinteger>();
  for (uinteger iter = 0u; iter < maxIter; ++iter)
  {
    // For each region
    for (uinteger i = 0; i < numRegions; ++i)
    {
      auto&& region = regions[i];
      // We group by quantized chroma value so that we can assume the same material
      static const uinteger numSlots = 20u;

      // Sum the intensities of all pixels that share the same chroma
      auto commonChromaIntensitySums = calculateCommonChromaIntensitySums(
          region,
          makeSpan(chroma, numPixels), 
          makeSpan(intensity, numPixels),
          numSlots,
          imageDimensions,
          regionScale
          );

      // Average the shading intensity
      fpreal shadingIntensitySum(0.0f);
      region.for_each_pixel([&](auto pixel, auto local)
          {
            shadingIntensitySum += (intensity[pixel] / region.m_albedoIntensities[local]);
          },
          imageDimensions, regionScale
      );
      auto shadingIntensityAverageRecip = shadingIntensitySum * (1.0f / numPixelsInRegion);

      region.for_each_pixel([&](auto, auto local)
          {
            region.m_albedoIntensities[local] = commonChromaIntensitySums[local] * shadingIntensityAverageRecip;
          },
          imageDimensions, regionScale
      );
    }
    std::cout<<"Maximisation Complete.\n";
    
    calcExpectedAlbedoIntensity(
        makeSpan(albedoIntensity, numPixels), 
        makeSpan(pixelRegions, numPixels), 
        imageDimensions, 
        regionScale
        );
    std::cout<<"Expectation Complete.\n";
  }

  auto shadingIntensity = std::make_unique<fpreal[]>(numPixels);
  for (uinteger i = 0; i < numPixels; ++i) 
  {
    shadingIntensity[i] = intensity[i] / albedoIntensity[i];
  }
  auto albedo = std::make_unique<fpreal3[]>(numPixels);
  for (uinteger i = 0; i < numPixels; ++i) 
  {
    albedo[i] = sourceImage[i] / shadingIntensity[i];
  }
  // Shading map is adjusted to use a 0.5 neutral rather than 1.0
  // This makes the shading detail much easier to view
  for (uinteger i = 0; i < numPixels; ++i) 
  {
    shadingIntensity[i] *= 0.5f;
  }


  const auto outputPrefix = args["output"].as<std::string>();
  const auto extension    = args["format"].as<std::string>();
  writeImage(outputPrefix + "_albedo." + extension , makeSpan(albedo, numPixels), imageDimensions);
  writeImage(outputPrefix + "_shading." + extension, makeSpan(shadingIntensity, numPixels), imageDimensions);
  return 0;
}
