
template <typename F>
void Region::for_each_pixel(F&& _func,
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
      _func(pixel, local);
    }
}
