#include "separation.h"
#include "image_util.h"
#include "util.h"

#include <glm/common.hpp>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <alloca.h>

BEGIN_AUTOTEXGEN_NAMESPACE

// Define our hashing algorithm as quantization of r and g into slots,
// to give slots^2 possible chroma values
uinteger hashChroma(const fpreal3 _chroma,
                    const fpreal3 _max,
                    const uinteger _slots) noexcept
{
  //uinteger x = glm::max(1.0f, glm::ceil(_chroma.r * 10.f));
  //uinteger y = glm::max(1.0f, glm::ceil(_chroma.g * 10.f));
  //return (x - 1.f) * 20.f + y;
   auto last  = _slots - 1;
   uinteger x = (_chroma.x / _max.x) * last;
   uinteger y = (_chroma.y / _max.y) * last;
   return y * last + x;
}

void estimateAlbedoIntensities(const Region _region,
                               fpreal* io_estimatedAlbedoIntensity,
                               const fpreal* _intensity,
                               const fpreal* _albedoIntensity,
                               const fpreal3* _chroma,
                               const fpreal3 _maxChroma,
                               const uinteger _numSlots,
                               const uint2 _imageDimensions,
                               const uinteger _regionScale) noexcept
{
  const uinteger numUniqueColors = _numSlots * _numSlots;
  auto contributions =
    static_cast<uinteger*>(alloca(numUniqueColors * sizeof(uinteger)));
  std::fill_n(contributions, numUniqueColors, 0u);
  // Average the shading intensity
  fpreal shadingIntensitySum(0.0f);
  for_each_local_pixel(
    [&](auto pixel, auto) {
      auto chromaId = hashChroma(_chroma[pixel], _maxChroma, _numSlots);
      io_estimatedAlbedoIntensity[chromaId] += _intensity[pixel];
      ++contributions[chromaId];
      shadingIntensitySum += (_intensity[pixel] / _albedoIntensity[pixel]);
    },
    _region,
    _imageDimensions,
    _regionScale);
  auto shadingIntensityAverage = shadingIntensitySum / (_regionScale*_regionScale);

  for (uinteger i = 0u; i < numUniqueColors; ++i)
  {
    if (contributions[i])
      io_estimatedAlbedoIntensity[i] /= (contributions[i] * shadingIntensityAverage);
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
  // TODO: Do not default construct
  auto albedoIntensity = std::make_unique<fpreal[]>(numPixels);
  std::copy_n(intensity.get(), numPixels, albedoIntensity.get());

  // Divide our images into regions,
  // we store the regions using pixel coordinates that represent their top left
  // pixel. We know the width and height is the same for each
  auto regionResult   = generateRegions(_imageDimensions, _regionScale);
  auto&& regions      = regionResult.m_regions;
  auto&& numRegionsXY = regionResult.m_numRegions;
  auto numRegions     = numRegionsXY.x * numRegionsXY.y;
  std::cout << numRegions << "Regions Complete.\n";

  fpreal3 maxChroma(0.0f);
  for (uinteger i = 0u; i < numPixels; ++i)
  {
    maxChroma = glm::max(maxChroma, chroma[i]);
  }
  const auto numSlots = 20u;
  for (uinteger iter = 0u; iter < _iterations; ++iter)
  {
    auto pixelContributions = std::make_unique<uinteger[]>(numPixels);
    auto interimAlbedoIntensity = std::make_unique<fpreal[]>(numPixels);
    // For each region
    for (uinteger i = 0; i < numRegions; ++i)
    {
      auto region = regions[i];
      auto estimatedAlbedoIntensity = std::make_unique<fpreal[]>(numSlots*numSlots);
      std::fill_n(estimatedAlbedoIntensity.get(), numSlots * numSlots, 0.0f);

      estimateAlbedoIntensities(region,
                                estimatedAlbedoIntensity.get(),
                                intensity.get(),
                                albedoIntensity.get(),
                                chroma.get(),
                                maxChroma,
                                numSlots,
                                _imageDimensions,
                                _regionScale);
      for_each_local_pixel(
        [&](auto pixel, auto) {
          auto chromaId      = hashChroma(chroma[pixel], maxChroma, numSlots);
          interimAlbedoIntensity[pixel] += estimatedAlbedoIntensity[chromaId];
          pixelContributions[pixel]++;
        },
        region,
        _imageDimensions,
        _regionScale);
    }
    std::cout << "Expectation Complete.\n";
    tbb::parallel_for(
        tbb::blocked_range<uinteger>{0u, numPixels},
        [&] (auto&& r) { 
          const auto end = r.end();
          for (auto i = r.begin(); i < end; ++i) 
          {
            albedoIntensity[i] = interimAlbedoIntensity[i] / pixelContributions[i];
          }});
    std::cout << "Maximisation Complete.\n";
  }

  
  // Calculate final albedo and shading maps from the seperated albedo intensity
  tbb::parallel_for(
      tbb::blocked_range<uinteger>{0u, numPixels},
      [&] (auto&& r) { 
        const auto end = r.end();
        for (auto i = r.begin(); i < end; ++i) 
        {
          io_shadingIntensity[i] = intensity[i] / albedoIntensity[i];
          io_albedo[i]           = albedoIntensity[i] * chroma[i];
        }});
}

END_AUTOTEXGEN_NAMESPACE
