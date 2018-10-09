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

}

int main()
{
  auto [source, dim] = readImage("images/conv.png");
  auto size = dim.x * dim.y;
  for (uinteger i = 0; i < size; ++i) source[i] = glm::clamp(source[i], fpreal3(25.f / 255.f), fpreal3(235.f / 255.f));
  auto intensity = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) intensity[i] = fpreal3((source[i].x + source[i].y + source[i].z) / 3);

  auto chroma = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) 
  {
    chroma[i].r = source[i].r / intensity[i].r;
    chroma[i].g = source[i].g / intensity[i].r;
    chroma[i].b = 3.0f - chroma[i].r - chroma[i].g;
  }
  auto shadingIntensity = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) shadingIntensity[i] = fpreal3(1.0f);
  auto albedoIntensity  = std::make_unique<fpreal3[]>(size);
  for (uinteger i = 0; i < size; ++i) albedoIntensity[i] = intensity[i];

  static constexpr uinteger w = 64;
  auto regionSize = w * w;
  auto regionDim = dim / w;
  auto numRegions = regionDim.x * regionDim.y;
  auto regions = std::make_unique<fpreal2[]>(numRegions);
  for (uinteger x = 0; x < regionDim.x; ++x)
  for (uinteger y = 0; y < regionDim.y; ++y)
  {
    regions[y * regionDim.x + x] = uint2{x * w, y * w};
  }

  for (uinteger i = 0; i < numRegions; ++i)
  {
    auto&& region = regions[i];

    static constexpr uinteger numSlots = 20u;
    static constexpr auto hashChroma = [](fpreal3 chroma, fpreal3 max)
    {
      auto last = numSlots - 1;
      uinteger x = (chroma.x / max.x) * last;
      uinteger y = (chroma.y / max.y) * last;
      return y * last + x;
    };
    std::array<std::vector<uint2>, numSlots * numSlots> chromaRegions;

    auto regionChroma = std::make_unique<fpreal3[]>(regionSize);
    auto maxChroma = fpreal3(0.f);
    for (uinteger x = 0; x < w; ++x)
    for (uinteger y = 0; y < w; ++y)
    {
      auto chromaVal = chroma[(region.y + y) * dim.y + (region.x + x)];
      auto regionPx = y * w + x;
      regionChroma[regionPx] = chromaVal;
      maxChroma = glm::max(maxChroma, chromaVal);
    }
    for (uinteger x = 0; x < w; ++x)
    for (uinteger y = 0; y < w; ++y)
    {
      auto regionPx = y * w + x;
      chromaRegions[hashChroma(regionChroma[regionPx], maxChroma)].push_back({x,y});
    } 

    for (int iters = 0; iters < 5; ++iters)
    {
      fpreal3 shadingIntensitySum(0.0f);
      for (uinteger x = 0; x < w; ++x)
      for (uinteger y = 0; y < w; ++y)
      {
        auto id = (region.y + y) * dim.y + (region.x + x);
        shadingIntensitySum += (intensity[id] / albedoIntensity[id]);
      }
      shadingIntensitySum /= regionSize;

      auto commonChromaIntensitySums = std::make_unique<fpreal3[]>(regionSize);
      for (uinteger k = 0; k < numSlots*numSlots; ++k)
      {
        fpreal3 sum(0.f);
        for (auto&& rpx : chromaRegions[k]) 
        {
          sum += intensity[(region.y + rpx.y) * dim.y + (region.x + rpx.x)];
        }
        sum /= chromaRegions[k].size();
        for (auto&& rpx : chromaRegions[k]) commonChromaIntensitySums[rpx.y * w + rpx.x] = sum;
      }

      for (uinteger x = 0; x < w; ++x)
      for (uinteger y = 0; y < w; ++y)
      {
        auto id = (region.y + y) * dim.y + (region.x + x);
        albedoIntensity[id] = commonChromaIntensitySums[y * w + x] / shadingIntensitySum;
      }
    }
  }

  for (uinteger i = 0; i < size; ++i) 
  {
    shadingIntensity[i] = intensity[i] / albedoIntensity[i];// - 0.6f;
  }


  writeImage("dump.ppm", nonstd::span<const fpreal3>{shadingIntensity.get(), size}, &dim.x);
  return 0;
}
