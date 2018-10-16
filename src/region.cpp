#include "region.h"

BEGIN_AUTOTEXGEN_NAMESPACE

uint2 Region::getPixelCoordFromLocal(uint2 _coord) const noexcept
{
  return {_coord.x + m_startPixel.x, _coord.y + m_startPixel.y};
}

uint2 Region::getLocalCoordFromPixel(uint2 _coord) const noexcept
{
  return {_coord.x - m_startPixel.x, _coord.y - m_startPixel.y};
}

fpreal* Region::getAlbedoIntensity(const uinteger _id,
                                   const uint2 _imageDim,
                                   const uinteger _regionScale)
{
  auto x = _id % _imageDim.y - m_startPixel.x;
  auto y = _id / _imageDim.y - m_startPixel.y;
  return &m_albedoIntensities[y * _regionScale + x];
}

RegionData generateRegions(const uint2 _imageDim,
                           const uinteger _regionScale,
                           const fpreal* const _albedoIntensities)
{
  RegionData r;
  // Number of regions in each axis, is equivalent to the size of the image in
  // those axis A, minus the last index within a Region (R-1) N = A - (R - 1)
  // <=> N = A - R + 1
  r.m_numRegions =
    uint2{_imageDim.x - _regionScale + 1, _imageDim.y - _regionScale + 1};
  // Store the total number of regions in the image
  auto totalNumRegions = r.m_numRegions.x * r.m_numRegions.y;
  // Number of pixels contained within a region
  auto numPixelsInRegion = _regionScale * _regionScale;
  // Allocate storage for the regions
  r.m_regions = std::make_unique<Region[]>(totalNumRegions);
  // Pixel regions contains a list of regions per pixel, which contain the pixel
  r.m_pixelRegions =
    std::make_unique<std::vector<Region*>[]>(_imageDim.x * _imageDim.y);

  for (uinteger x = 0u; x < r.m_numRegions.x; ++x)
    for (uinteger y = 0u; y < r.m_numRegions.y; ++y)
    {
      // Construct our region
      auto&& region = r.m_regions[y * r.m_numRegions.x + x];
      region = Region{{x, y}, std::make_unique<fpreal[]>(numPixelsInRegion)};
      // For every pixel in the region, we push a pointer to this region, into
      // their pixelRegion list
      for (uinteger px = x; px < x + _regionScale; ++px)
        for (uinteger py = y; py < y + _regionScale; ++py)
        { r.m_pixelRegions[py * _imageDim.x + px].push_back(&region); }
        // Init the albedo for our new region
      region.for_each_pixel(
        [&](auto pixel, auto local) {
          region.m_albedoIntensities[local] = _albedoIntensities[pixel];
        },
        _imageDim,
        _regionScale);
    }
  // return our regions, and pixel regions
  return r;
}

END_AUTOTEXGEN_NAMESPACE
