#ifndef INCLUDED_REGION_H
#define INCLUDED_REGION_H

#include "types.h"

#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

struct Region
{
  uint2 m_startPixel;
  std::unique_ptr<fpreal[]> m_albedoIntensities;

  uint2 getPixelCoordFromLocal(uint2 _coord) const noexcept;

  fpreal* getAlbedoIntensity(const uinteger _id,
                             const uint2 _imageDim,
                             const uinteger _regionScale);

  template <typename F>
  void for_each_pixel(F&& _func,
                      const uint2 _imageDim,
                      const uinteger _regionScale) const noexcept;
  
};

struct RegionData
{
  std::unique_ptr<Region[]> m_regions;
  std::unique_ptr<std::vector<Region*>[]> m_pixelRegions;
  uint2 m_numRegions;
};
RegionData generateRegions(const uint2 _imageDim,
                           const uinteger _regionScale,
                           const fpreal* const _albedoIntensities);

#include "region.inl"

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_REGION_H