#ifndef INCLUDED_IMAGEUTILS_H
#define INCLUDED_IMAGEUTILS_H

#include "types.h"

#include <OpenImageIO/imageio.h>

BEGIN_AUTOTEXGEN_NAMESPACE

void clampExtremeties(span<fpreal> io_image);

void clampExtremeties(span<fpreal3> io_image);

std::vector<fpreal> calculateIntensity(const span<fpreal3> _image);

std::vector<fpreal3> calculateChroma(const span<fpreal3> _sourceImage,
                                           const span<fpreal> _intensity);

template <typename T, typename E = fpreal>
void writeImage(const string_view _filename,
                const T* _data,
                const uinteger2 _imageDim);

template <typename T, typename E = fpreal>
auto readImage(const string_view _filename);

#include "image_util.inl"  //template definitions

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_IMAGEUTILS_H
