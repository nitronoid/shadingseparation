#ifndef INCLUDED_REGION_H
#define INCLUDED_REGION_H

#include "types.h"

#include <vector>

struct Region
{
  uint2 m_startPixel;
  std::unique_ptr<fpreal[]> m_albedoIntensities;

  uint2 getPixelCoordFromLocal(uint2 _coord) const noexcept;

  fpreal* getAlbedoIntensity(uinteger _id,
                             const uint2 _imageDim,
                             const uinteger _regionScale);

  template <typename F>
  void for_each_pixel(F&& func,
                      const uint2 _imageDim,
                      const uinteger _regionScale) const noexcept
  {
    for (uinteger x = 0u; x < _regionScale; ++x)
      for (uinteger y = 0u; y < _regionScale; ++y)
      {
        uint2 localCoord{x, y};
        auto local      = localCoord.y * _regionScale + localCoord.x;
        auto pixelCoord = getPixelCoordFromLocal(localCoord);
        auto pixel      = pixelCoord.y * _imageDim.x + pixelCoord.x;
        func(pixel, local);
      }
  }
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

#endif  // INCLUDED_REGION_H
