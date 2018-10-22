#ifndef INCLUDED_REGION_H
#define INCLUDED_REGION_H

#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

using Region = uint2;

struct RegionData
{
  std::unique_ptr<Region[]> m_regions;
  uint2 m_numRegions;
};
RegionData generateRegions(const uint2 _imageDim,
                           const uinteger _regionScale);

template <typename F>
void for_each_local_pixel(F&& _func,
                          const uint2 _regionStart,
                          const uint2 _imageDim,
                          const uinteger _regionScale) noexcept;

#include "region.inl"

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_REGION_H
