#ifndef INCLUDED_SPECULAR_H
#define INCLUDED_SPECULAR_H

#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<std::vector<uinteger>> initMaterialSets(const span<fpreal3> _albedo,
                                                    uinteger2 _imageDim,
                                                    uinteger _numSets);

void removeOutliers(span<std::vector<uinteger>> _materialTypes,
                    const span<fpreal3> _albedo);

std::vector<std::vector<fpreal>>
computeProbability(const span<std::vector<uinteger>> _materialSets,
                   const span<fpreal3> _albedo);

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_SPECULAR_H
