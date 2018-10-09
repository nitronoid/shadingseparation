#include <QImage>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "types.h"
#include <nonstd/span.hpp>
#include <string_view>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace
{
std::ostream &operator<< (std::ostream &out, const uint3 &vec) 
{
  out<<vec.r<<' '<<vec.g<<' '<<vec.b;
  return out;
}
// convert RGB float in range [0,1] to int in range [0, 255] and perform gamma correction
inline uint3 quantize(fpreal3 x)
{ 
  static constexpr fpreal gamma = 1.0 / 2.2;
  static const fpreal3 gamma3(gamma);
  return uint3(glm::pow(x, gamma3) * 255.0f + 0.5f); 
}  

void writeImage(std::string_view filename, nonstd::span<const fpreal3> _data, const uinteger* dim)
{
  // Write image to PPM file, a very simple image file format
  std::ofstream outFile;
  outFile.open(filename.data());
  outFile<<"P3\n"<<dim[0]<<" "<<dim[1]<<"\n255\n";
  const auto size = _data.size();
  for (uinteger i = 0u; i < size; i++)  // loop over pixels, write RGB values
    outFile<<quantize(_data[i])<<'\n';
}
}

int main()
{
  QImage input("/home/s4902673/Documents/cpp/appgen/images/rust.png");
  uint2 dim {input.width(), input.height()};
  auto size = dim.x * dim.y;
  auto data = std::make_unique<fpreal3[]>(size);
  static constexpr auto div = 1.f / 255.f;
  for (uinteger x = 0u; x < dim.x; ++x)
  for (uinteger y = 0u; y < dim.y; ++y)
  {
    auto col = input.pixelColor(x, y);
    int3 icol;
    col.getRgb(&icol.x, &icol.y, &icol.z); 
    data[y * dim.x + x] = fpreal3{icol.x * div, icol.y * div, icol.z * div}; 
  }
  std::cout<<"CONV\n";
  writeImage("dump.ppm", nonstd::span<const fpreal3>{data.get(), size}, &dim.x);
  return 0;
}
