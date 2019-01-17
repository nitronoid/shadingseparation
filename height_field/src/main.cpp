#include "image_util.h"
#include "separation.h"
#include "normal.h"
#include "types.h"
#include "util.h"

#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_square_root.hpp>

namespace
{
inline static auto getParser()
{
  cxxopts::Options parser("Shading Separator",
                          "Implementation of microsofts Appgen");
  // clang-format off
  parser.allow_unrecognised_options().add_options() 
    ("h,help", "Print help") 
    ("s,shading-map", "Shading map file name", cxxopts::value<std::string>()) 
    ("o,output", "Output file name",    cxxopts::value<std::string>()->default_value("height_map.png")) 
    ("a,azimuth", "Azimuthal angle of lighting direction", cxxopts::value<atg::fpreal>()->default_value("45")) 
    ("p,polar", "Polar angle of lighting direction", cxxopts::value<atg::fpreal>()->default_value("45")) 
    ;
  // clang-format on
  return parser;
}

}  // namespace

int main(int argc, char* argv[])
{
  using namespace atg;
  // Parse the commandline options
  auto parser     = getParser();
  const auto args = parser.parse(argc, argv);
  if (args.count("help") || !args.count("shading-map"))
  {
    std::cout << parser.help() << '\n';
    std::exit(0);
  }

  // Obtain spherical coordinates, r is assumed to be 1 as this is a direction
  auto azimuth = glm::radians(args["azimuth"].as<fpreal>());
  auto polar = glm::radians(args["polar"].as<fpreal>());
  // Compute cartesian direction vector from our spherical coordinates
  fpreal3 L = {
    glm::sin(polar) * glm::cos(azimuth),
    glm::sin(polar) * glm::sin(azimuth),
    glm::cos(polar)};
  L = glm::fastNormalize(L);
  std::cout<<L.x<<' '<<L.y<<' '<<L.z<<'\n';

  // Read the source image in as an array of rgbf
  auto imgResult =
    readImage<fpreal>(args["shading-map"].as<std::string>());
  auto&& sourceImageData = imgResult.m_data;
  auto&& imageDimensions = imgResult.m_imageDim;
  auto numPixels         = imageDimensions.x * imageDimensions.y;
  auto shadingImage = makeSpan(sourceImageData, numPixels);

  clampExtremeties(shadingImage);

  auto normals = computeRelativeNormals(shadingImage, L);
  auto rh = computeRelativeHeights(normals.data(), imageDimensions);
  auto h = computeAbsoluteHeights(rh.data(), imageDimensions);
  writeImage(args["output"].as<std::string>(),
             h.data(),
             imageDimensions);

  return 0;
}
