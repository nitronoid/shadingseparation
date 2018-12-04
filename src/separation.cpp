#include "separation.h"
#include "image_util.h"
#include "util.h"
#include "filter.h"

#include <glm/common.hpp>
#include <glm/gtx/extended_min_max.hpp>

#include <alloca.h>
#include <numeric>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

BEGIN_AUTOTEXGEN_NAMESPACE

// Define our hashing algorithm as quantization of r and g into slots,
// to give slots^2 possible chroma values
uinteger hashChroma(const fpreal3 _chroma,
                    const fpreal3 _max,
                    const uinteger _slots) noexcept
{
  // uinteger x = glm::max(1.0_f, glm::ceil(_chroma.r * 10.0_f));
  // uinteger y = glm::max(1.0_f, glm::ceil(_chroma.g * 10.0_f));
  // return (x - 1.0_f) * 20.0_f + y;
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
                               const uinteger2 _imageDimensions,
                               const uinteger _regionScale) noexcept
{
  const uinteger numPixels       = _regionScale * _regionScale;
  const uinteger numUniqueColors = _numSlots * _numSlots;
  auto contributions =
    static_cast<uinteger*>(alloca(numUniqueColors * sizeof(uinteger)));
  std::fill_n(contributions, numUniqueColors, 0u);
  // Average the shading intensity
  fpreal shadingIntensitySum(0.0_f);
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
  auto shadingIntensityAverage = shadingIntensitySum / numPixels;

  for (uinteger i = 0u; i < numUniqueColors; ++i)
  {
    if (contributions[i])
      io_estimatedAlbedoIntensity[i] /=
        (contributions[i] * shadingIntensityAverage);
  }
}

namespace
{
uinteger2 calculateContributions(uinteger2 _coord, uinteger2 _regionDim, uinteger2 _dim)
{
  auto coord = _coord + 1u;
  return glm::min(_regionDim, coord, _dim - _coord);
}
}

void seperateShading(const span<fpreal3> _sourceImage,
                     fpreal3* io_albedo,
                     fpreal* io_shadingIntensity,
                     const uinteger2 _imageDimensions,
                     const uinteger _regionScale,
                     const uinteger _directIterations,
                     const uinteger _intensityIterations,
                     const uinteger _chromaSlots)
{
  auto numPixels = _imageDimensions.x * _imageDimensions.y;
  auto intensity = calculateIntensity(_sourceImage);
  // Extract the chroma of the image using our intensity
  auto chroma = calculateChroma(_sourceImage, intensity);
  // Our shading intensity defaults to one, so albedo intensity = source
  // intensity i = si * ai
  auto albedoIntensity = intensity;
  std::fill_n(io_shadingIntensity, numPixels, 1.0_f);

  // Divide our images into regions,
  // we store the regions using pixel coordinates that represent their top left
  // pixel. We know the width and height is the same for each
  auto regionResult   = generateRegions(_imageDimensions, _regionScale);
  auto&& regions      = regionResult.m_regions;
  auto&& numRegionsXY = regionResult.m_numRegions;
  auto numRegions     = numRegionsXY.x * numRegionsXY.y;
  std::cout << "Region generation complete: " << numRegions << " created.\n";

  // Find the largest chroma value
  auto maxChroma = std::accumulate(chroma.begin(), chroma.end(), fpreal3(0.0_f),
      [](const auto& a, const auto& b)
      {
        return glm::max(a, b);
      });

  const uinteger totalNumSlots = _chromaSlots * _chromaSlots;
  const auto filter = gaussianFilter(uinteger2(_regionScale));

  for (uinteger resetNum = 0u; resetNum < _directIterations; ++resetNum)
  {
    // Reset the intensity to the albedo intensity every step
    intensity = albedoIntensity;
    for (uinteger iter = 0u; iter < _intensityIterations; ++iter)
    {
      std::cout << "\33[2K\rIteration " << 
        iter + _intensityIterations * resetNum + 1 << ". " << std::flush;

      std::vector<uinteger> pixelContributions(numPixels, 0u);
      std::vector<fpreal> interimAlbedoIntensity(numPixels, 0.0_f);
      // For each region
      for (uinteger i = 0; i < numRegions; ++i)
      {
        auto region = regions[i];
        std::vector<fpreal> estimatedAlbedoIntensity(totalNumSlots, 0.0_f);
        estimateAlbedoIntensities(region,
                                  estimatedAlbedoIntensity.data(),
                                  intensity.data(),
                                  albedoIntensity.data(),
                                  chroma.data(),
                                  maxChroma,
                                  _chromaSlots,
                                  _imageDimensions,
                                  _regionScale);
        for_each_local_pixel(
          [&](auto pixel, auto local) {
            auto chromaId = hashChroma(chroma[pixel], maxChroma, _chromaSlots);
            auto w = filter[local];
            interimAlbedoIntensity[pixel] += estimatedAlbedoIntensity[chromaId] * w;
          },
          region,
          _imageDimensions,
          _regionScale);
      }

      tbb::parallel_for(tbb::blocked_range<uinteger>{0u, numPixels},
                        [&](auto&& r) {
                          const auto end = r.end();
                          for (auto i = r.begin(); i < end; ++i)
                          {
                            uinteger2 pixelCoord{i%_imageDimensions.x, i/_imageDimensions.x};
                            auto contrib = calculateContributions(pixelCoord, uinteger2(_regionScale), _imageDimensions);
                            auto pixelContributions = filterSum(filter, uinteger2(_regionScale), contrib);
                            albedoIntensity[i] =
                              interimAlbedoIntensity[i] / (pixelContributions);
                          }
                        });
    }
    // Calculate shading intensity
    tbb::parallel_for(
      tbb::blocked_range<uinteger>{0u, numPixels}, [&](auto&& r) {
        const auto end = r.end();
        for (auto i = r.begin(); i < end; ++i)
        {
          io_shadingIntensity[i] += (intensity[i] / albedoIntensity[i] - 1.0_f);
        }
      });
  }
  std::cout<<"\33[2K\r" << _directIterations * _intensityIterations << 
    " Iterations completed.\n"<<std::flush;

  // Calculate final albedo
  tbb::parallel_for(tbb::blocked_range<uinteger>{0u, numPixels}, [&](auto&& r) {
    const auto end = r.end();
    for (auto i = r.begin(); i < end; ++i)
    {
      io_albedo[i] = albedoIntensity[i] * chroma[i];
    }
  });
}

END_AUTOTEXGEN_NAMESPACE
