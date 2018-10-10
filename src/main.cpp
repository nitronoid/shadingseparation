#include <QImage>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "types.h"
#include <nonstd/span.hpp>
#include <string_view>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>

namespace
{
std::ostream &operator<< (std::ostream &out, const fpreal3 &vec) 
{
  out<<vec.r<<' '<<vec.g<<' '<<vec.b;
  return out;
}
std::ostream &operator<< (std::ostream &out, const uint3 &vec) 
{
  out<<vec.r<<' '<<vec.g<<' '<<vec.b;
  return out;
}
// convert RGB float in range [0,1] to int in range [0, 255] and perform gamma correction
inline uint3 quantize(fpreal3 x)
{ 
  static constexpr fpreal gamma = 1.0;// / 2.2;
  static const fpreal3 gamma3(gamma);
  return uint3(glm::pow(x, gamma3) * 255.0f + 0.5f); 
}  

void writeImage(std::string_view _filename, nonstd::span<const fpreal3> _data, const uinteger* _dim)
{
  // Write image to PPM file, a very simple image file format
  std::ofstream outFile;
  outFile.open(_filename.data());
  outFile<<"P3\n"<<_dim[0]<<" "<<_dim[1]<<"\n255\n";
  const auto size = _data.size();
  for (uinteger i = 0u; i < size; i++)  // loop over pixels, write RGB values
    outFile<<quantize(_data[i])<<'\n';
}

void writeImage(std::string_view _filename, nonstd::span<const fpreal> _data, const uinteger* _dim)
{
  // Write image to PPM file, a very simple image file format
  std::ofstream outFile;
  outFile.open(_filename.data());
  outFile<<"P3\n"<<_dim[0]<<" "<<_dim[1]<<"\n255\n";
  const auto size = _data.size();
  for (uinteger i = 0u; i < size; i++)  // loop over pixels, write RGB values
    outFile<<quantize(fpreal3(_data[i]))<<'\n';
}

auto readImage(std::string_view _filename)
{
  QImage input(_filename.data());
  uint2 dim {input.width(), input.height()};
  auto data = std::make_unique<fpreal3[]>(dim.x * dim.y);
  static constexpr auto div = 1.f / 255.f;
  for (uinteger x = 0u; x < dim.x; ++x)
  for (uinteger y = 0u; y < dim.y; ++y)
  {
    auto col = input.pixelColor(x, y);
    int3 icol;
    col.getRgb(&icol.x, &icol.y, &icol.z); 
    data[y * dim.x + x] = fpreal3{icol.x * div, icol.y * div, icol.z * div}; 
  }
  // return an owning span, 
  struct OwningSpan
  {
    std::unique_ptr<fpreal3[]> data;
    uint2 dim;
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
  void for_each_pixel(F&& func, const uinteger _regionScale) const noexcept
  {
    for (uinteger x = 0u; x < _regionScale; ++x)
    for (uinteger y = 0u; y < _regionScale; ++y)
    {
      uint2 local {x, y};
      func(getPixelCoordFromLocal(local), local);
    }
  }
};


auto generateRegions(const uint2 _imageDim, const uinteger _regionScale)
{
  uint2 regionDim {_imageDim.x - _regionScale +1, _imageDim.y - _regionScale +1};
  auto numRegions = regionDim.x * regionDim.y;
  auto regionSize = _regionScale * _regionScale;
  std::unique_ptr<Region[]> regions(new Region[numRegions]);
  auto pixelRegions = std::make_unique<std::vector<Region*>[]>(_imageDim.x * _imageDim.y);
  auto regionPtr = regions.get();

  for (uinteger x = 0u; x < regionDim.x; ++x) 
  for (uinteger y = 0u; y < regionDim.y; ++y)
  {
    auto newRegionPtr = regionPtr + (y * regionDim.x + x);
    new(newRegionPtr) Region{{x, y}, std::make_unique<fpreal[]>(regionSize)};
    for (uinteger px = x; px < x + _regionScale; ++px)
    for (uinteger py = y; py < y + _regionScale; ++py)
    {
      pixelRegions[py * _imageDim.x + px].push_back(newRegionPtr);
    }
  }
  // return an owning span, 
  struct RegionData
  {
    decltype(regions) m_regions;
    decltype(pixelRegions) m_pixelRegions;
    decltype(regionDim) m_dim;
  };
  return RegionData{std::move(regions), std::move(pixelRegions), std::move(regionDim)};
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

}

int main()
{
  // Read the source image in as an array of rgbf
  auto [source, dim] = readImage("images/paper.png");
  auto size = dim.x * dim.y;
  // Remove highlights and shadows
  for (uinteger i = 0; i < size; ++i) source[i] = glm::clamp(source[i], fpreal3(25.f / 255.f), fpreal3(235.f / 255.f));
  // Extract the intensity as the average of rgb
  static constexpr fpreal third = 1.0f / 3.0f;
  auto intensity = std::make_unique<fpreal[]>(size);
  for (uinteger i = 0; i < size; ++i) intensity[i] = (source[i].x + source[i].y + source[i].z) * third;

  // Extract the chroma of the image using out intensity
  // {r/i, g/i, 3 - r/i - g/i}
  auto chroma = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) 
  {
    chroma[i].r = source[i].r / intensity[i];
    chroma[i].g = source[i].g / intensity[i];
    chroma[i].b = 3.0f - chroma[i].r - chroma[i].g;
  }
  // Our shading intensity defaults to one, so that albedo intensity can be = intensity
  // i = si * ai
  auto shadingIntensity = std::make_unique<fpreal[]>(size);
  for (uinteger i = 0; i < size; ++i) shadingIntensity[i] = 1.0f;
  auto albedoIntensity  = std::make_unique<fpreal[]>(size);
  for (uinteger i = 0; i < size; ++i) albedoIntensity[i] = intensity[i];

  // Divide our images into regions, we store the regions as coordinates,
  // as we know the width and height is the same for each
  const uinteger regionScale = 20u;
  const uinteger regionSize  = regionScale * regionScale;
  auto [regions, pixelRegions, regionDim] = generateRegions(dim, regionScale);
  auto numRegions = regionDim.x * regionDim.y;

  std::cout<<"Regions Complete.\n";
    for (uinteger i = 0u; i < numRegions; ++i) 
    {
      regions[i].for_each_pixel([&](uint2 _coord, uint2 _regionLocalCoord)
          {
            auto pixelId = _coord.y * dim.x + _coord.x;
            auto id = _regionLocalCoord.y * regionScale + _regionLocalCoord.x;
            regions[i].m_albedoIntensities[id] = albedoIntensity[pixelId];
          },
          regionScale
      );
    }

  for (int iter = 0; iter < 10; ++iter)
  {

    // For each region
    for (uinteger i = 0; i < numRegions; ++i)
    {
      auto&& region = regions[i];
      // We group by quantized chroma value so that we can assume the same material
      static constexpr uinteger numSlots = 20u;
      std::vector<std::vector<uint2>> quantizedChromas;
      quantizedChromas.resize(numSlots * numSlots);

      // Group using the chromas, storing the index into the current region that gives us the real pixel
      // We first copy the chroma values for the region into a local array, and simultaneously find the
      // maximum chroma value for quantization
      auto regionChromas = std::make_unique<fpreal3[]>(regionSize);
      auto maxChroma = fpreal3(0.f);
      region.for_each_pixel([&](uint2 _coord, uint2 _regionLocalCoord)
          {
            auto chromaVal = chroma[_coord.y * dim.x + _coord.x];
            auto regionPx = _regionLocalCoord.y * regionScale + _regionLocalCoord.x;
            regionChromas[regionPx] = chromaVal;
            maxChroma = glm::max(maxChroma, chromaVal);
          },
          regionScale
      );
      
      // Now we hash our copied chroma values
      region.for_each_pixel([&](uint2 _coord, uint2 _regionLocalCoord)
          {
            auto regionPx = _regionLocalCoord.y * regionScale + _regionLocalCoord.x;
            auto qChromaHash = hashChroma(std::move(regionChromas[regionPx]), maxChroma, numSlots);
            quantizedChromas[qChromaHash].push_back(_regionLocalCoord);
          },
          regionScale
      );

      // Average the shading intensity
      fpreal shadingIntensitySum(0.0f);
      region.for_each_pixel([&](uint2 _coord, uint2 _regionLocalCoord)
          {
            auto id = _coord.y * dim.x + _coord.x;
            auto regionId = _regionLocalCoord.y * regionScale + _regionLocalCoord.x;
            shadingIntensitySum += (intensity[id] / region.m_albedoIntensities[regionId]);
          },
          regionScale
      );
      auto shadingIntensityAverageRecip = shadingIntensitySum * (1.0f / regionSize);

      auto commonChromaIntensitySums = std::make_unique<fpreal[]>(regionSize);
      for (auto&& qChroma : quantizedChromas)
      {
        fpreal sum(0.f);
        for (auto&& rpx : qChroma) 
        {
          auto pixelCoord = region.getPixelCoordFromLocal(rpx);
          sum += intensity[pixelCoord.y * dim.x + pixelCoord.x];
        }
        auto averageChromaIntensity = sum / static_cast<fpreal>(qChroma.size());
        for (auto&& rpx : qChroma) 
        {
          commonChromaIntensitySums[rpx.y * regionScale + rpx.x] = averageChromaIntensity;
        }
      }

      region.for_each_pixel([&](uint2 _coord, uint2 _regionLocalCoord)
          {
            auto id = _coord.y * dim.x + _coord.x;
            auto regionId = _regionLocalCoord.y * regionScale + _regionLocalCoord.x;
            region.m_albedoIntensities[regionId] = commonChromaIntensitySums[regionId] * shadingIntensityAverageRecip;
          },
          regionScale
      );
    }
    std::cout<<"Maximisation Complete.\n";
    
    // For each pixel
    for (uinteger pixelId = 0; pixelId < size; ++pixelId)
    {
      // get average of albedo intensity for all regions containing this pixel
      fpreal totalAlbedoIntensity = 0.f;
      for (auto rptr : pixelRegions[pixelId])
      {
        totalAlbedoIntensity += *rptr->getAlbedoIntensity(pixelId, dim, regionScale);
      }
      // Store the albedo intensity for the current pixel
      albedoIntensity[pixelId] = totalAlbedoIntensity/ pixelRegions[pixelId].size();
      for (auto rptr : pixelRegions[pixelId])
      {
        // Update the stored albedo intensity for each region
        *rptr->getAlbedoIntensity(pixelId, dim, regionScale) = albedoIntensity[pixelId];
      }
    }
    std::cout<<"Expectation Complete.\n";
#if DEBUG_OUTPUT_CONVERGENCE
    std::filesystem::create_directory("test");
    auto nom = std::string("test/foo_0") + std::to_string(iter) + ".ppm";
    std::cout<<nom<<'\n';
    writeImage(nom, nonstd::span<const fpreal>{albedoIntensity.get(), size}, &dim.x);
#endif //DEBUG_OUTPUT_CONVERGENCE
  }

  for (uinteger i = 0; i < size; ++i) 
  {
    shadingIntensity[i] = intensity[i] / albedoIntensity[i];
  }
  auto albedo = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) 
  {
    albedo[i] = source[i] / shadingIntensity[i];
  }
  for (uinteger i = 0; i < size; ++i) 
  {
    shadingIntensity[i] *= 0.5f;
  }


  writeImage("albedoDump.ppm", nonstd::span<const fpreal3>{albedo.get(), size}, &dim.x);
  writeImage("shadingDump.ppm", nonstd::span<const fpreal>{shadingIntensity.get(), size}, &dim.x);
  writeImage("pDump.ppm", nonstd::span<const fpreal>{albedoIntensity.get(), size}, &dim.x);
  return 0;
}
