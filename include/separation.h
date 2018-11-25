#ifndef INCLUDED_SEPARATION_H
#define INCLUDED_SEPARATION_H

#include "region.h"
#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

uinteger hashChroma(const fpreal3 _chroma,
                    const fpreal3 _max,
                    const uinteger _slots) noexcept;

void estimateAlbedoIntensities(const Region _region,
                               fpreal* io_estimatedAlbedoIntensity,
                               const fpreal* _intensity,
                               const fpreal* _albedoIntensity,
                               const fpreal3* _chroma,
                               const fpreal3 _maxChroma,
                               const uinteger _numSlots,
                               const uinteger2 _imageDimensions,
                               const uinteger _regionScale) noexcept;

void seperateShading(const span<fpreal3> _sourceImage,
                     fpreal3* io_albedo,
                     fpreal* io_shadingIntensity,
                     const uinteger2 _imageDimensions,
                     const uinteger _regionScale,
                     const uinteger _directIterations,
                     const uinteger _intensityIterations,
                     const uinteger _chromaSlots);

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_SEPARATION_H
