#ifndef INCLUDED_NORMAL_H
#define INCLUDED_NORMAL_H

#include "types.h"
#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal3> computeRelativeNormals(const_span<fpreal> _shading, const fpreal3 _lightDirection);

std::vector<fpreal2> computeRelativeHeights(fpreal3* _normals, uinteger2 _imageDim);

std::vector<fpreal> computeAbsoluteHeights(fpreal2* _relativeHeights, uinteger2 _imageDim);

END_AUTOTEXGEN_NAMESPACE

#endif//INCLUDED_NORMAL_H
