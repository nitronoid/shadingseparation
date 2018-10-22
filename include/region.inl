
template <typename F>
void for_each_local_pixel(F&& _func,
                          const uint2 _regionStart,
                          const uint2 _imageDim,
                          const uinteger _regionScale) noexcept
{
  for (uinteger x = 0u; x < _regionScale; ++x)
    for (uinteger y = 0u; y < _regionScale; ++y)
    {
      uint2 localCoord{x, y};
      auto local      = localCoord.y * _regionScale + localCoord.x;
      auto pixelCoord = localCoord + _regionStart;
      auto pixel      = pixelCoord.y * _imageDim.x + pixelCoord.x;
      _func(pixel, local);
    }
}
