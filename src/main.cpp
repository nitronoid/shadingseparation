#include "types.h"
#include <iomanip>
#include <iostream>
#include <cxxopts.hpp>

#include "separation.h"
#include "image_util.h"
#include "util.h"

namespace
{

inline static auto getParser()
{
  cxxopts::Options parser("Shading Separator", "Implementation of microsofts Appgen");
  parser.allow_unrecognised_options().add_options()
  ("h,help", "Print help")
  ("s,source", "Source file name", cxxopts::value<std::string>())
  ("o,output", "Output file name (no extension)", cxxopts::value<std::string>()->default_value("shading"))
  ("f,format", "Output file format (the extension)", cxxopts::value<std::string>()->default_value("png"))
  ("r,region", "Region scale", cxxopts::value<uinteger>()->default_value("20"))
  ("i,iterations", "Seperation iterations", cxxopts::value<uinteger>()->default_value("3"))
  ;
  return parser;
}

}

int main(int argc, char* argv[])
{
  // Parse the commandline options
  auto parser = getParser();
  const auto args = parser.parse(argc, argv);
  if (args.count("help") || !args.count("source"))
  {
    std::cout<<parser.help()<<'\n';
    std::exit(0);
  }

  // Read the source image in as an array of rgbf
  auto imgResult = readImage<fpreal3>(args["source"].as<std::string>());
  auto&& sourceImageData = imgResult.m_data;
  auto&& imageDimensions = imgResult.m_imageDim;
  auto numPixels = imageDimensions.x * imageDimensions.y;
  auto sourceImage = makeSpan(sourceImageData, numPixels);

  // Remove the extreme highlights and shadows by clamping intense pixels
  clampExtremeties(sourceImage);

  // Allocated arrays to store the resulting textures
  auto albedo = std::make_unique<fpreal3[]>(numPixels);
  auto shadingIntensity = std::make_unique<fpreal[]>(numPixels);

  const uinteger regionScale = args["region"].as<uinteger>();
  const auto maxIter = args["iterations"].as<uinteger>();
  // Split out the albedo and shading from the source image
  seperateShading(sourceImage, albedo.get(), shadingIntensity.get(), imageDimensions, regionScale, maxIter);

  // Shading map is adjusted to use a 0.5 neutral rather than 1.0
  // This makes the shading detail much easier to view
  for (uinteger i = 0; i < numPixels; ++i) shadingIntensity[i] *= 0.5f;

  const auto outputPrefix = args["output"].as<std::string>();
  const auto extension    = args["format"].as<std::string>();
  writeImage(outputPrefix + "_albedo." + extension , makeSpan(albedo, numPixels), imageDimensions);
  writeImage(outputPrefix + "_shading." + extension, makeSpan(shadingIntensity, numPixels), imageDimensions);
  return 0;
}

