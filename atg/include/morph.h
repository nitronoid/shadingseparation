#ifndef INCLUDED_MORPH_H
#define INCLUDED_MORPH_H

#include "types.h"

#include <numeric>
#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

void erode(fpreal* _in,
           fpreal* o_out,
           uinteger2 _imageDim,
           uinteger2 _structuringElement,
           uinteger _iter);

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_MORPH_H
