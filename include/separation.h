#ifndef INCLUDED_SEPARATION_H
#define INCLUDED_SEPARATION_H

#include "region.h"
#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

uinteger
hashChroma(const fpreal3 _chroma, const fpreal3 _max, const uinteger _slots) noexcept;

void quantizeChromas(const Region& _region,
                     span<std::vector<uint2>> o_quantizedChromas,
                     const span<fpreal3> _chromas,
                     const uinteger _numSlots,
                     const uint2 _imageDimensions,
                     const uinteger _regionScale);

std::unique_ptr<fpreal[]>
calculateCommonChromaIntensitySums(const Region& _region,
                                   const span<fpreal3> _chroma,
                                   const span<fpreal> _intensity,
                                   const uinteger _numSlots,
                                   const uint2 _imageDimensions,
                                   const uinteger _regionScale);

void calcExpectedAlbedoIntensity(span<fpreal> io_albedoIntensity,
                                 span<std::vector<Region*>> _pixelRegions,
                                 const uint2 _imageDimensions,
                                 const uinteger _regionScale);

void seperateShading(const span<fpreal3> _sourceImage,
                     fpreal3* io_albedo,
                     fpreal* io_shadingIntensity,
                     const uint2 _imageDimensions,
                     const uinteger _regionScale,
                     const uinteger _iterations);

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_SEPARATION_H
