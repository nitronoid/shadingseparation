#ifndef INCLUDED_NORMAL_H
#define INCLUDED_NORMAL_H

#include "types.h"
#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal3> computeHeightMap(const_span<fpreal> _shading, const fpreal3 _lightDirection);

END_AUTOTEXGEN_NAMESPACE

#endif//INCLUDED_NORMAL_H
