#ifndef INCLUDED_FILTER_H
#define INCLUDED_FILTER_H

#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal> gaussianFilter(uinteger2 _dim, fpreal sigma = 1.0_f);

fpreal filterSum(const span<const fpreal> _filter, const uinteger2 _dimensions);

fpreal filterSum(const span<const fpreal> _filter, const uinteger2 _dimensions, const uinteger2 _crop);

END_AUTOTEXGEN_NAMESPACE

#endif//INCLUDED_FILTER_H
