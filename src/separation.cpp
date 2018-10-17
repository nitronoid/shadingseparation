#include "separation.h"
#include "image_util.h"
#include "util.h"

#include <alloca.h>
#include <glm/common.hpp>

BEGIN_AUTOTEXGEN_NAMESPACE

// Define our hashing algorithm as quantization of r and g into slots,
// to give slots^2 possible chroma values
uinteger hashChroma(const fpreal3 _chroma,
                    const fpreal3 _max,
                    const uinteger _slots) noexcept
{
  auto last  = _slots - 1;
  uinteger x = (_chroma.x / _max.x) * last;
  uinteger y = (_chroma.y / _max.y) * last;
  return y * last + x;
}

void estimateAlbedos(Region* io_region, const fpreal* _intensity, const fpreal* _albedoIntensity, const fpreal3* _chroma, const fpreal3 _maxChroma, const uinteger _numSlots, const uint2 _imageDimensions, const uinteger _regionScale) noexcept
{
  const uinteger numUniqueColors = _numSlots * _numSlots;
  auto contributions = static_cast<uinteger*>(alloca(numUniqueColors * sizeof(uinteger)));
  std::fill_n(contributions, numUniqueColors, 0u);
  // Average the shading intensity
  fpreal shadingIntensitySum(0.0f);
  io_region->for_each_pixel(
    [&](auto pixel, auto) {
      auto chromaId = hashChroma(_chroma[pixel], _maxChroma, _numSlots);
      io_region->m_expectedAlbedoIntensity[chromaId] += _intensity[pixel];
      ++contributions[chromaId];
      shadingIntensitySum += (_intensity[pixel] / _albedoIntensity[pixel]);
    },
    _imageDimensions,
    _regionScale);
  auto shadingIntensityAverage = shadingIntensitySum / numUniqueColors;

  for (uinteger i = 0u; i < numUniqueColors; ++i)
  {
    if (contributions[i]) 
      io_region->m_expectedAlbedoIntensity[i] /= (contributions[i] * shadingIntensityAverage);
    else 
      io_region->m_expectedAlbedoIntensity[i] = 0.0f;
  }
}

void seperateShading(const span<fpreal3> _sourceImage,
                     fpreal3* io_albedo,
                     fpreal* io_shadingIntensity,
                     const uint2 _imageDimensions,
                     const uinteger _regionScale,
                     const uinteger _iterations)
{
  auto numPixels = _imageDimensions.x * _imageDimensions.y;
  auto intensity = calculateIntensity(_sourceImage);
  // Extract the chroma of the image using our intensity
  auto chroma = calculateChroma(_sourceImage, makeSpan(intensity, numPixels));
  // Our shading intensity defaults to one, so albedo intensity = source
  // intensity i = si * ai
  // //TODO: Do not default construct
  auto albedoIntensity = std::make_unique<fpreal[]>(numPixels);
  std::memcpy(albedoIntensity.get(), intensity.get(), sizeof(fpreal)*numPixels);

  // Divide our images into regions,
  // we store the regions using pixel coordinates that represent their top left
  // pixel. We know the width and height is the same for each
  auto regionResult =
    generateRegions(_imageDimensions, _regionScale, albedoIntensity.get());
  auto&& regions      = regionResult.m_regions;
  auto&& pixelRegions = regionResult.m_pixelRegions;
  auto&& numRegionsXY = regionResult.m_numRegions;
  auto numRegions     = numRegionsXY.x * numRegionsXY.y;
  std::cout << "Regions Complete.\n";
  fpreal3 maxChroma(0.0f);
  for (uinteger i = 0u; i < numPixels; ++i)
  {
    maxChroma = glm::max(maxChroma, chroma[i]);
  }

  for (uinteger iter = 0u; iter < _iterations; ++iter)
  {
    // For each region
    for (uinteger i = 0; i < numRegions; ++i)
    {
      auto&& region = regions[i];
      std::fill_n(region.m_expectedAlbedoIntensity.get(), 400u, 0.0f);
      estimateAlbedos(&region, intensity.get(), albedoIntensity.get(), chroma.get(), maxChroma, 20u, _imageDimensions, _regionScale);

    }
    std::cout << "Expectation Complete.\n";
    for (uinteger i = 0u; i < numPixels; ++i)
    {
      fpreal sum = 0.0;
      for (auto rptr : pixelRegions[i])
      {
        auto chromaId = hashChroma(chroma[i], maxChroma, 20u);
        sum += rptr->m_expectedAlbedoIntensity[chromaId];
      }
      albedoIntensity[i] = sum / pixelRegions[i].size();
    }
    std::cout << "Maximisation Complete.\n";
  }

  for (uinteger i = 0; i < numPixels; ++i)
  {
    io_shadingIntensity[i] = intensity[i] / albedoIntensity[i];
    io_albedo[i]           = albedoIntensity[i] * chroma[i];
  }
}

END_AUTOTEXGEN_NAMESPACE
