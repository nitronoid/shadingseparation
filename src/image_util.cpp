#include "image_util.h"

#include <glm/common.hpp>

BEGIN_AUTOTEXGEN_NAMESPACE

void clampExtremeties(span<fpreal3> io_image)
{
  // TODO: take caps as input
  // Remove highlights and shadows
  static const fpreal3 shadowCap(25.f / 255.f);
  static const fpreal3 highlightCap(235.f / 255.f);
  for (auto& pixel : io_image)
  {
    pixel = glm::clamp(pixel, shadowCap, highlightCap);
  }
}

std::unique_ptr<fpreal[]> calculateIntensity(const span<fpreal3> _image)
{
  auto numPixels = _image.size();
  // Extract the intensity as the average of rgb
  static constexpr fpreal third = 1.0f / 3.0f;
  // TODO: do not default construct
  auto intensity = std::make_unique<fpreal[]>(numPixels);
  for (uinteger i = 0; i < numPixels; ++i)
  {
    auto& pixel  = _image[i];
    intensity[i] = (pixel.x + pixel.y + pixel.z) * third;
  }
  return intensity;
}

std::unique_ptr<fpreal3[]> calculateChroma(const span<fpreal3> _sourceImage,
                                           const span<fpreal> _intensity)
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

END_AUTOTEXGEN_NAMESPACE
