#include "image_util.h"

#include <glm/common.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

BEGIN_AUTOTEXGEN_NAMESPACE

void clampExtremeties(span<fpreal3> io_image)
{
  // TODO: take caps as input
  // Remove highlights and shadows
  static const fpreal3 shadowCap(1.f / 255.f);
  static const fpreal3 highlightCap(254.f / 255.f);
  for (auto& pixel : io_image)
  {
    pixel = glm::clamp(pixel, shadowCap, highlightCap);
  }
}

std::unique_ptr<fpreal[]> calculateIntensity(const span<fpreal3> _image)
{
  uinteger numPixels = _image.size();
  // TODO: do not default construct
  auto intensity = std::make_unique<fpreal[]>(numPixels);
  tbb::parallel_for(tbb::blocked_range<atg::uinteger>{0u, numPixels},
                    [&intensity, &_image](auto&& r) {
                      // Extract the intensity as the average of rgb
                      static constexpr fpreal third = 1.0f / 3.0f;
                      const auto end                = r.end();
                      for (auto i = r.begin(); i < end; ++i)
                      {
                        auto& pixel  = _image[i];
                        intensity[i] = (pixel.x + pixel.y + pixel.z) * third;
                      }
                    });
  return intensity;
}

std::unique_ptr<fpreal3[]> calculateChroma(const span<fpreal3> _sourceImage,
                                           const span<fpreal> _intensity)
{
  uinteger numPixels = _sourceImage.size();
  // {r/i, g/i, 3 - r/i - g/i}
  auto chroma = std::make_unique<fpreal3[]>(numPixels);
  tbb::parallel_for(tbb::blocked_range<atg::uinteger>{0u, numPixels},
                    [&](auto&& r) {
                      const auto end = r.end();
                      for (auto i = r.begin(); i < end; ++i)
                      {
                        chroma[i].r = _sourceImage[i].r / _intensity[i];
                        chroma[i].g = _sourceImage[i].g / _intensity[i];
                        chroma[i].b = 3.0f - chroma[i].r - chroma[i].g;
                      }
                    });
  return chroma;
}

END_AUTOTEXGEN_NAMESPACE
