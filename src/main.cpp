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
  using namespace atg;
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
    readImage<fpreal3>(args["source"].as<std::string>());
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
                       args["intensity-iterations"].as<uinteger>(),
                       args["quantize-slots"].as<uinteger>());

  const auto outputPrefix = args["output"].as<std::string>();
  const auto extension    = args["format"].as<std::string>();

  auto normals = computeRelativeNormals(makeSpan(shadingIntensity, numPixels), {-0.5_f, 0.5_f, 1._f});
  writeImage(outputPrefix + "_normals." + extension,
                  normals.data(),
                  imageDimensions);
  auto rh = computeRelativeHeights(normals.data(), imageDimensions);
  auto h = computeAbsoluteHeights(rh.data(), imageDimensions);
  writeImage(outputPrefix + "_h." + extension,
                  h.data(),
                  imageDimensions);

  // Shading map is adjusted to use a 0.5 neutral rather than 1.0
  // This makes the shading detail much easier to view
  tbb::parallel_for(tbb::blocked_range<uinteger>{0u, numPixels},
                    [&shadingIntensity](auto&& r) {
                      const auto end = r.end();
                      for (auto i = r.begin(); i < end; ++i)
                        shadingIntensity[i] *= 0.5_f;
                    });

  writeImage(outputPrefix + "_albedo." + extension,
                  albedo.get(),
                  imageDimensions);
  writeImage(outputPrefix + "_shading." + extension,
                  shadingIntensity.get(),
                  imageDimensions);



  auto gtAlbedo = makeSpan(albedo, numPixels);

  const uint numSets = 2u;
  // Loading sets from stroke images
  // auto set0img = readImage<fpreal3>("images/sets/set0.png");
  // auto set0 = makeSpan(set0img.m_data, numPixels);
  // auto set1img = readImage<fpreal3>("images/sets/set1.png");
  // auto set1 = makeSpan(set1img.m_data, numPixels);
  // std::vector<std::vector<uinteger>> materialSets(numSets);
  // materialSets[0].reserve(numPixels);
  // materialSets[1].reserve(numPixels);
  // for (uint i = 0u; i < numPixels; ++i)
  //{
  //  if(set0[i].x > 0.0_f) materialSets[0].push_back(i);
  //  if(set1[i].x > 0.0_f) materialSets[1].push_back(i);
  //}

  auto materialSets = initMaterialSets(gtAlbedo, imageDimensions, numSets);
  removeOutliers(materialSets, gtAlbedo);
  auto probabilities = computeProbability(materialSets, gtAlbedo);

  for (uint i = 0u; i < numSets; ++i)
  {
    writeImage(outputPrefix + "_probability_" + std::to_string(i) + "." +
                      extension,
                    probabilities[i].data(),
                    imageDimensions);
  }

  float coef[2] = { 0.1f, 0.6f };
  std::vector<fpreal> roughness(numPixels);
  for (uint i = 0u; i < numPixels; ++i)
    roughness[i] = probabilities[0][i] * coef[0] + probabilities[1][i] * coef[1];
  writeImage(outputPrefix + "_roughness." + extension,
                  roughness.data(),
                  imageDimensions);

  float coefs[2] = { 0.99f, 0.2f };
  std::vector<fpreal> specular(numPixels);
  for (uint i = 0u; i < numPixels; ++i)
    specular[i] = probabilities[0][i] * coefs[0] + probabilities[1][i] * coefs[1];
  writeImage(outputPrefix + "_specular." + extension,
                  specular.data(),
                  imageDimensions);

  auto img = std::make_unique<fpreal[]>(numPixels);
  for (uint i = 0u; i < numSets; ++i)
  {
    std::fill_n(img.get(), numPixels, 0.0_f);
    for (auto p : materialSets[i])
      img[p] = 1.0_f;

    writeImage(outputPrefix + "_material_set_" + std::to_string(i) + "." +
                      extension,
                    img.get(),
                    imageDimensions);
  }

  return 0;
}
