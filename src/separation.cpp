#include "separation.h"

#include "util.h"

#include <glm/common.hpp>

// Define our hashing algorithm as quantization of r and g into slots,
// to give slots^2 possible chroma values
uinteger hashChroma(fpreal3 _chroma, fpreal3 _max, uinteger _slots) noexcept
{
  auto last  = _slots - 1;
  uinteger x = (_chroma.x / _max.x) * last;
  uinteger y = (_chroma.y / _max.y) * last;
  return y * last + x;
}

void quantizeChromas(const Region& _region,
                     span<std::vector<uint2>> o_quantizedChromas,
                     span<fpreal3> _chromas,
                     const uinteger _numSlots,
                     const uint2 _imageDimensions,
                     const uinteger _regionScale)
{
  // Group using the chromas, storing the index into the current region that
  // gives us the real pixel We first copy the chroma values for the region into
  // a local array, and simultaneously find the maximum chroma value for
  // quantization
  auto maxChroma = fpreal3(0.f);
  _region.for_each_pixel(
    [&](auto pixel, auto) {
      auto chromaVal = _chromas[pixel];
      maxChroma      = glm::max(maxChroma, chromaVal);
    },
    _imageDimensions,
    _regionScale);

  // Now we hash our copied chroma values
  _region.for_each_pixel(
    [&](auto pixel, auto local) {
      auto qChromaHash = hashChroma(_chromas[pixel], maxChroma, _numSlots);
      o_quantizedChromas[qChromaHash].emplace_back(local % _regionScale,
                                                   local / _regionScale);
    },
    _imageDimensions,
    _regionScale);
}

std::unique_ptr<fpreal[]>
calculateCommonChromaIntensitySums(const Region& _region,
                                   span<fpreal3> _chroma,
                                   span<fpreal> _intensity,
                                   const uinteger _numSlots,
                                   const uint2 _imageDimensions,
                                   const uinteger _regionScale)
{
  static const uinteger numUniqueChromas = _numSlots * _numSlots;
  auto quantizedChromaData =
    std::make_unique<std::vector<uint2>[]>(numUniqueChromas);
  auto quantizedChromas = makeSpan(quantizedChromaData, numUniqueChromas);
  quantizeChromas(_region,
                  quantizedChromas,
                  _chroma,
                  _numSlots,
                  _imageDimensions,
                  _regionScale);

  auto commonChromaIntensitySums =
    std::make_unique<fpreal[]>(_regionScale * _regionScale);
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
      commonChromaIntensitySums[rpx.y * _regionScale + rpx.x] =
        averageChromaIntensity;
    }
  }
  return commonChromaIntensitySums;
}

void calcExpectedAlbedoIntensity(span<fpreal> io_albedoIntensity,
                                 span<std::vector<Region*>> _pixelRegions,
                                 const uint2 _imageDimensions,
                                 const uinteger _regionScale)
{
  auto numPixels = io_albedoIntensity.size();
  // For each pixel
  for (uinteger pixelId = 0; pixelId < numPixels; ++pixelId)
  {
    // get average of albedo intensity for all regions containing this pixel
    fpreal totalAlbedoIntensity = 0.f;
    for (auto rptr : _pixelRegions[pixelId])
    {
      totalAlbedoIntensity +=
        *rptr->getAlbedoIntensity(pixelId, _imageDimensions, _regionScale);
    }
    // Store the albedo intensity for the current pixel
    io_albedoIntensity[pixelId] =
      totalAlbedoIntensity / _pixelRegions[pixelId].size();
    for (auto rptr : _pixelRegions[pixelId])
    {
      // Update the stored albedo intensity for each region
      *rptr->getAlbedoIntensity(pixelId, _imageDimensions, _regionScale) =
        io_albedoIntensity[pixelId];
    }
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
  std::memcpy(albedoIntensity.get(),
              intensity.get(),
              sizeof(albedoIntensity[0]) * numPixels);

  // Divide our images into regions,
  // we store the regions using pixel coordinates that represent their top left
  // pixel. We know the width and height is the same for each
  const uinteger numPixelsInRegion = _regionScale * _regionScale;
  auto regionResult =
    generateRegions(_imageDimensions, _regionScale, albedoIntensity.get());
  auto&& regions      = regionResult.m_regions;
  auto&& pixelRegions = regionResult.m_pixelRegions;
  auto&& numRegionsXY = regionResult.m_numRegions;
  auto numRegions     = numRegionsXY.x * numRegionsXY.y;
  std::cout << "Regions Complete.\n";

  for (uinteger iter = 0u; iter < _iterations; ++iter)
  {
    // For each region
    for (uinteger i = 0; i < numRegions; ++i)
    {
      auto&& region = regions[i];
      // We group by quantized chroma value so that we can assume the same
      // material
      static const uinteger numSlots = 20u;

      // Sum the intensities of all pixels that share the same chroma
      auto commonChromaIntensitySums =
        calculateCommonChromaIntensitySums(region,
                                           makeSpan(chroma, numPixels),
                                           makeSpan(intensity, numPixels),
                                           numSlots,
                                           _imageDimensions,
                                           _regionScale);

      // Average the shading intensity
      fpreal shadingIntensitySum(0.0f);
      region.for_each_pixel(
        [&](auto pixel, auto local) {
          shadingIntensitySum +=
            (intensity[pixel] / region.m_albedoIntensities[local]);
        },
        _imageDimensions,
        _regionScale);
      auto shadingIntensityAverageRecip =
        shadingIntensitySum * (1.0f / numPixelsInRegion);

      region.for_each_pixel(
        [&](auto, auto local) {
          region.m_albedoIntensities[local] =
            commonChromaIntensitySums[local] * shadingIntensityAverageRecip;
        },
        _imageDimensions,
        _regionScale);
    }
    std::cout << "Maximisation Complete.\n";

    calcExpectedAlbedoIntensity(makeSpan(albedoIntensity, numPixels),
                                makeSpan(pixelRegions, numPixels),
                                _imageDimensions,
                                _regionScale);
    std::cout << "Expectation Complete.\n";
  }

  for (uinteger i = 0; i < numPixels; ++i)
  {
    io_shadingIntensity[i] = intensity[i] / albedoIntensity[i];
    io_albedo[i]           = _sourceImage[i] / io_shadingIntensity[i];
  }
}
