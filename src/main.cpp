#include "image_util.h"
#include "separation.h"
#include "types.h"
#include "util.h"

#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>

namespace
{
inline static auto getParser()
{
  cxxopts::Options parser("Shading Separator",
                          "Implementation of microsofts Appgen");
  // clang-format off
  parser.allow_unrecognised_options().add_options() 
    ("h,help", "Print help") 
    ("s,source", "Source file name", cxxopts::value<std::string>()) 
    ("o,output", "Output file name (no extension)",    cxxopts::value<std::string>()->default_value("shading")) 
    ("f,format", "Output file format (the extension)", cxxopts::value<std::string>()->default_value("png")) 
    ("r,region", "Region scale", cxxopts::value<atg::uinteger>()->default_value("20")) 
    ("i,iterations", "Seperation iterations", cxxopts::value<atg::uinteger>()->default_value("3"))
    ;
  // clang-format on
  return parser;
}

}  // namespace

int main(int argc, char* argv[])
{
  // Parse the commandline options
  auto parser     = getParser();
  const auto args = parser.parse(argc, argv);
  if (args.count("help") || !args.count("source"))
  {
    std::cout << parser.help() << '\n';
    std::exit(0);
  }

  // Read the source image in as an array of rgbf
  auto imgResult         = atg::readImage<atg::fpreal3>(args["source"].as<std::string>());
  auto&& sourceImageData = imgResult.m_data;
  auto&& imageDimensions = imgResult.m_imageDim;
  auto numPixels         = imageDimensions.x * imageDimensions.y;
  auto sourceImage       = atg::makeSpan(sourceImageData, numPixels);

  // Remove the extreme highlights and shadows by clamping intense pixels
  atg::clampExtremeties(sourceImage);

  // Allocated arrays to store the resulting textures
  auto albedo           = std::make_unique<atg::fpreal3[]>(numPixels);
  auto shadingIntensity = std::make_unique<atg::fpreal[]>(numPixels);

  // Split out the albedo and shading from the source image
  atg::seperateShading(sourceImage,
                  albedo.get(),
                  shadingIntensity.get(),
                  imageDimensions,
                  args["region"].as<atg::uinteger>(),
                  args["iterations"].as<atg::uinteger>());

  // Shading map is adjusted to use a 0.5 neutral rather than 1.0
  // This makes the shading detail much easier to view
  for (atg::uinteger i = 0; i < numPixels; ++i) shadingIntensity[i] *= 0.5f;

  const auto outputPrefix = args["output"].as<std::string>();
  const auto extension    = args["format"].as<std::string>();
  atg::writeImage(outputPrefix + "_albedo." + extension,
             atg::makeSpan(albedo, numPixels),
             imageDimensions);
  atg::writeImage(outputPrefix + "_shading." + extension,
             atg::makeSpan(shadingIntensity, numPixels),
             imageDimensions);
  return 0;
}
