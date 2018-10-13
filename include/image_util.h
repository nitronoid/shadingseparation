#ifndef INCLUDED_IMAGEUTILS_H
#define INCLUDED_IMAGEUTILS_H

#include "types.h"

#include <OpenImageIO/imageio.h>

void clampExtremeties(span<fpreal3> io_image);

std::unique_ptr<fpreal[]> calculateIntensity(span<fpreal3> _image);

std::unique_ptr<fpreal3[]> calculateChroma(span<const fpreal3> _sourceImage,
                                           span<const fpreal> _intensity);

template <typename T>
void writeImage(string_view _filename, span<T> _data, const uint2 _imageDim)
{
  std::cout << "Writing image to " << _filename << '\n';
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageOutput, void (*)(ImageOutput*)> output(
    ImageOutput::create(_filename.data()), [](auto ptr) {
      ptr->close();
      delete ptr;
    });
  ImageSpec is(
    _imageDim.x, _imageDim.y, sizeof(T) / sizeof(fpreal), TypeDesc::FLOAT);
  output->open(_filename.data(), is);
  output->write_image(TypeDesc::FLOAT, _data.data());
}

template <typename T>
auto readImage(string_view _filename)
{
  // OpenImageIO namespace
  using namespace OIIO;
  // unique_ptr with custom deleter to close file on exit
  std::unique_ptr<ImageInput, void (*)(ImageInput*)> input(
    ImageInput::open(_filename.data()), [](auto ptr) {
      ptr->close();
      delete ptr;
    });
  // Get the image specification and store the dimensions
  auto&& spec = input->spec();
  uint2 dim{spec.width, spec.height};

  // Allocated an array for our data
  auto data = std::make_unique<T[]>(dim.x * dim.y);
  // Read the data into our array, with a specified stride of how many floats
  // the user requested, i.e. for fpreal3 we ignore the alpha channel
  input->read_image(
    TypeDesc::FLOAT, data.get(), sizeof(T), AutoStride, AutoStride);

  // return an owning span
  struct OwningSpan
  {
    std::unique_ptr<T[]> m_data;
    uint2 m_imageDim;
  };
  return OwningSpan{std::move(data), std::move(dim)};
}

#endif  // INCLUDED_IMAGEUTILS_H
