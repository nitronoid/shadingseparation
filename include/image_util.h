#ifndef INCLUDED_IMAGEUTILS_H
#define INCLUDED_IMAGEUTILS_H

#include "types.h"

#include <OpenImageIO/imageio.h>

BEGIN_AUTOTEXGEN_NAMESPACE

void clampExtremeties(span<fpreal3> io_image);

std::unique_ptr<fpreal[]> calculateIntensity(const span<fpreal3> _image);

std::unique_ptr<fpreal3[]> calculateChroma(const span<fpreal3> _sourceImage,
                                           const span<fpreal> _intensity);

template <typename T>
void writeImage(const string_view _filename,
                const span<T> _data,
                const uint2 _imageDim);

template <typename T>
auto readImage(const string_view _filename);

#include "image_util.inl" //template definitions

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_IMAGEUTILS_H
