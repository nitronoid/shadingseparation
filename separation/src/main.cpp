#include "image_util.h"
#include "separation.h"
#include "specular.h"
#include "normal.h"
#include "types.h"
#include "util.h"

#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace
{
inline static auto getParser()
{
  cxxopts::Options parser("Shading Separator",
                          "Intrinsic image decomposition");
  // clang-format off
  parser.allow_unrecognised_options().add_options() 
    ("h,help", "Print help") 
    ("i,input-image", "Source file name", cxxopts::value<std::string>()) 
    ("a,albedo-output", "Albedo map output file name",   cxxopts::value<std::string>()->default_value("albedo.png")) 
    ("s,shading-output", "Shading map output file name", cxxopts::value<std::string>()->default_value("shading.png")) 
    ("r,region", "Region scale", cxxopts::value<atg::uinteger>()->default_value("10")) 
    ("q,quantize-slots", "Chroma quantization slots", cxxopts::value<atg::uinteger>()->default_value("10"))
    ("e,expectation-iterations", "Intensity seperation iterations", cxxopts::value<atg::uinteger>()->default_value("5"))
    ("d,direct-iterations", "Direct seperation iterations", cxxopts::value<atg::uinteger>()->default_value("5"))
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
  if (args.count("help") || !args.count("input-image"))
  {
    std::cout << parser.help() << '\n';
    std::exit(0);
  }

  // Read the source image in as an array of rgbf
  auto imgResult =
    readImage<fpreal3>(args["input-image"].as<std::string>());
  auto&& sourceImageData = imgResult.m_data;
  auto&& imageDimensions = imgResult.m_imageDim;
  auto numPixels         = imageDimensions.x * imageDimensions.y;
  auto sourceImage       = makeSpan(sourceImageData, numPixels);

  // Remove the extreme highlights and shadows by clamping intense pixels
  clampExtremeties(sourceImage);

  // Allocated arrays to store the resulting textures
  auto albedo           = std::make_unique<fpreal3[]>(numPixels);
  auto shadingIntensity = std::make_unique<fpreal[]>(numPixels);

  // Split out the albedo and shading from the source image
  seperateShading(sourceImage,
                  albedo.get(),
                  shadingIntensity.get(),
                  imageDimensions,
                  args["region"].as<uinteger>(),
                  args["direct-iterations"].as<uinteger>(),
                  args["expectation-iterations"].as<uinteger>(),
                  args["quantize-slots"].as<uinteger>());

  writeImage(args["albedo-output"].as<std::string>(),
                  albedo.get(),
                  imageDimensions);
  // Shading map should be adjusted to use a 0.5 neutral rather than 1.0,
  // for easier viewing
  writeImage(args["shading-output"].as<std::string>(),
                  shadingIntensity.get(),
                  imageDimensions);


  return 0;
}
