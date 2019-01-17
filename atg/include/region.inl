
template <typename F>
void for_each_local_pixel2D(F&& _func,
                          const uinteger2 _regionStart,
                          const uinteger _regionScale) noexcept
{
  for (uinteger x = 0u; x < _regionScale; ++x)
    for (uinteger y = 0u; y < _regionScale; ++y)
    {
      uinteger2 localCoord{x, y};
      auto pixelCoord = localCoord + _regionStart;
      _func(pixelCoord, localCoord);
    }
}

template <typename F>
void for_each_local_pixel(F&& _func,
                          const uinteger2 _regionStart,
                          const uinteger2 _imageDim,
                          const uinteger _regionScale) noexcept
{
  for_each_local_pixel2D([&]
      (auto pixelCoord, auto localCoord)
      {
        auto local = localCoord.y * _regionScale + localCoord.x;
        auto pixel = pixelCoord.y * _imageDim.x + pixelCoord.x;
        _func(pixel, local);
      },
      _regionStart,
      _regionScale);
}
