#include "morph.h"

BEGIN_AUTOTEXGEN_NAMESPACE

namespace
{
uinteger2 computeBounds(uinteger _coord, uinteger _width, uinteger _cap)
{
  return {std::max(0, int(_coord) - int(_width)),
          std::min(_cap, _coord + _width)};
}
}  // namespace

void erode(fpreal* _in,
           fpreal* o_out,
           uinteger2 _imageDim,
           uinteger2 _structuringElement,
           uinteger _iter)
{
  const uinteger2 kernelHalf =
    ((_structuringElement - uinteger2(1u)) * _iter) / 2u;
  for (uinteger y = 0u; y < _imageDim.y; ++y)
  {
    for (uinteger x = 0u; x < _imageDim.x; ++x)
    {
      const uinteger offset = y * _imageDim.x;
      fpreal w              = 1.f;
      auto yBounds          = computeBounds(y, kernelHalf.y, _imageDim.y);
      auto xBounds          = computeBounds(x, kernelHalf.x, _imageDim.x);
      for (uinteger n = yBounds.x; n < yBounds.y; ++n)
      {
        w *= std::accumulate(_in + (n * _imageDim.x) + xBounds.x,
                             _in + (n * _imageDim.x) + xBounds.y,
                             1.f,
                             std::multiplies<fpreal>());
      }
      o_out[offset + x] = w;
    }
  }
}

END_AUTOTEXGEN_NAMESPACE
