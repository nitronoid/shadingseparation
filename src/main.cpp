#include "image_util.h"
#include "separation.h"
#include "types.h"
#include "util.h"
#include "specular.h"

#include <cxxopts.hpp>
#include <iomanip>
#include <iostream>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>


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
    ("r,region", "Region scale", cxxopts::value<atg::uinteger>()->default_value("10")) 
    ("q,quantize-slots", "Chroma quantization slots", cxxopts::value<atg::uinteger>()->default_value("10"))
    ("i,intensity-iterations", "Intensity seperation iterations", cxxopts::value<atg::uinteger>()->default_value("5"))
    ("d,direct-iterations", "Direct seperation iterations", cxxopts::value<atg::uinteger>()->default_value("5"))
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
  auto imgResult =
    atg::readImage<atg::fpreal3>(args["source"].as<std::string>());
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
                       args["direct-iterations"].as<atg::uinteger>(),
                       args["intensity-iterations"].as<atg::uinteger>(),
                       args["quantize-slots"].as<atg::uinteger>());

  // Shading map is adjusted to use a 0.5 neutral rather than 1.0
  // This makes the shading detail much easier to view
  tbb::parallel_for(
      tbb::blocked_range<atg::uinteger>{0u, numPixels},
      [&shadingIntensity] (auto&& r) { 
        const auto end = r.end();
        for (auto i = r.begin(); i < end; ++i) 
          shadingIntensity[i] *= 0.5f; 
        });

  const auto outputPrefix = args["output"].as<std::string>();
  const auto extension    = args["format"].as<std::string>();
  atg::writeImage(outputPrefix + "_albedo." + extension,
                  atg::makeSpan(albedo, numPixels),
                  imageDimensions);
  atg::writeImage(outputPrefix + "_shading." + extension,
                  atg::makeSpan(shadingIntensity, numPixels),
                  imageDimensions);


  auto gtAlbedo = atg::makeSpan(albedo, numPixels);

  const uint numSets = 2u;
  // Loading sets from stroke images
  //auto set0img = atg::readImage<atg::fpreal3>("images/sets/set0.png");
  //auto set0 = atg::makeSpan(set0img.m_data, numPixels);
  //auto set1img = atg::readImage<atg::fpreal3>("images/sets/set1.png");
  //auto set1 = atg::makeSpan(set1img.m_data, numPixels);
  //std::vector<std::vector<atg::uinteger>> materialSets(numSets);
  //materialSets[0].reserve(numPixels);
  //materialSets[1].reserve(numPixels);
  //for (uint i = 0u; i < numPixels; ++i)
  //{
  //  if(set0[i].x > 0.f) materialSets[0].push_back(i);
  //  if(set1[i].x > 0.f) materialSets[1].push_back(i);
  //}

  auto materialSets = atg::initMaterialSets(gtAlbedo, imageDimensions, numSets);
  atg::removeOutliers(materialSets, gtAlbedo);
  auto probabilities = atg::computeProbability(materialSets, gtAlbedo);

  for (uint i = 0u; i < numSets; ++i)
  {
    atg::writeImage(outputPrefix + "_probability_" + std::to_string(i) + "." + extension,
                    atg::span<atg::fpreal>(probabilities[i]),
                    imageDimensions);
  }

  auto img = std::make_unique<atg::fpreal[]>(numPixels);
  for (uint i = 0u; i < numSets; ++i)
  {
    std::fill_n(img.get(), numPixels, 0.0f);
    for (auto p : materialSets[i]) img[p] = 1.f; 

    atg::writeImage(outputPrefix + "_material_set_" + std::to_string(i) + "." + extension,
                    atg::makeSpan(img, numPixels),
                    imageDimensions);
  }


  return 0;
}
