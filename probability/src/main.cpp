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
                          "Implementation of microsofts Appgen");
  // clang-format off
  parser.allow_unrecognised_options().add_options() 
    ("h,help", "Print help") 
    ("i,input-image", "Source file name", cxxopts::value<std::string>()) 
    ("o,output", "Output file name",    cxxopts::value<std::string>()->default_value("probability_map.png")) 
    ("s,sets", "Number of material sets that exhibit distinct specular properties", cxxopts::value<atg::uinteger>())
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
  if (args.count("help") || !args.count("input-image") || !args.count("sets"))
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

  const uint numSets = args["sets"].as<uinteger>();

  auto materialSets = initMaterialSets(sourceImage, imageDimensions, numSets);
  removeOutliers(materialSets, sourceImage);
  auto probabilities = computeProbability(materialSets, sourceImage);

  auto outName = args["output"].as<std::string>();
  auto extPos = outName.find('.');
  std::string prefix = outName.substr(0, extPos);
  std::string ext = outName.substr(extPos, outName.size());
  for (uint i = 0u; i < numSets; ++i)
  {
    writeImage(prefix + std::to_string(i) + ext,
               probabilities[i].data(),
               imageDimensions);
  }

  //auto img = std::make_unique<fpreal[]>(numPixels);
  //for (uint i = 0u; i < numSets; ++i)
  //{
  //  std::fill_n(img.get(), numPixels, 0.0_f);
  //  for (auto p : materialSets[i])
  //    img[p] = 1.0_f;

  //  writeImage(outputPrefix + "_material_set_" + std::to_string(i) + "." +
  //                    extension,
  //                  img.get(),
  //                  imageDimensions);
  //}

  return 0;
}
