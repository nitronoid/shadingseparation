#include "region.h"

BEGIN_AUTOTEXGEN_NAMESPACE

RegionData generateRegions(const uint2 _imageDim,
                           const uinteger _regionScale)
{
  RegionData r;
  // Number of regions in each axis, is equivalent to the size of the image in
  // those axis A, minus the last index within a Region (R-1) N = A - (R - 1)
  // <=> N = A - R + 1
  r.m_numRegions =
    uint2{_imageDim.x - _regionScale + 1, _imageDim.y - _regionScale + 1};
  // Store the total number of regions in the image
  auto totalNumRegions = r.m_numRegions.x * r.m_numRegions.y;
  // Allocate storage for the regions
  r.m_regions = std::make_unique<Region[]>(totalNumRegions);

  for (uinteger x = 0u; x < r.m_numRegions.x; ++x)
    for (uinteger y = 0u; y < r.m_numRegions.y; ++y)
    {
      // Construct our region
      auto& region = r.m_regions[y * r.m_numRegions.x + x];
      region       = Region{x, y};
    }
  // return our regions, and pixel regions
  return r;
}

END_AUTOTEXGEN_NAMESPACE
