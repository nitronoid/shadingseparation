#include "image_util.h"

#include <glm/common.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

BEGIN_AUTOTEXGEN_NAMESPACE

void clampExtremeties(span<fpreal3> io_image)
{
  // TODO: take caps as input
  // Remove highlights and shadows
  static const fpreal3 shadowCap(1.0_f / 255.0_f);
  static const fpreal3 highlightCap(254.0_f / 255.0_f);
  for (auto& pixel : io_image)
  {
    pixel = glm::clamp(pixel, shadowCap, highlightCap);
  }
}

std::vector<fpreal> calculateIntensity(const span<fpreal3> _image)
{
  uinteger numPixels = _image.size();
  // TODO: do not default construct
  std::vector<fpreal> intensity(numPixels);
  tbb::parallel_for(tbb::blocked_range<atg::uinteger>{0u, numPixels},
                    [&intensity, &_image](auto&& r) {
                      // Extract the intensity as the average of rgb
                      static constexpr fpreal third = 1.0_f / 3.0_f;
                      const auto end                = r.end();
                      for (auto i = r.begin(); i < end; ++i)
                      {
                        auto& pixel  = _image[i];
                        intensity[i] = (pixel.x + pixel.y + pixel.z) * third;
                      }
                    });
  return intensity;
}

std::vector<fpreal3> calculateChroma(const span<fpreal3> _sourceImage,
                                           const span<fpreal> _intensity)
{
  uinteger numPixels = _sourceImage.size();
  // {r/i, g/i, 3 - r/i - g/i}
  std::vector<fpreal3> chroma(numPixels);
  tbb::parallel_for(tbb::blocked_range<atg::uinteger>{0u, numPixels},
                    [&](auto&& r) {
                      const auto end = r.end();
                      for (auto i = r.begin(); i < end; ++i)
                      {
                        chroma[i].r = _sourceImage[i].r / _intensity[i];
                        chroma[i].g = _sourceImage[i].g / _intensity[i];
                        chroma[i].b = 3.0_f - chroma[i].r - chroma[i].g;
                      }
                    });
  return chroma;
}

END_AUTOTEXGEN_NAMESPACE
