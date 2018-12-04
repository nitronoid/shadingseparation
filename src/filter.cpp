#include "filter.h"
#include <numeric>
#include <cmath>

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal> gaussianFilter(uinteger2 _dim, fpreal sigma) 
{ 
  const auto sqr = [](const auto x) { return x*x; };
  std::vector<fpreal> filter(_dim.x * _dim.y);

  const fpreal2 mid = (static_cast<fpreal2>(_dim) - 1.0_f) * 0.5_f;

  const fpreal sigmaSqr = sqr(sigma);
  const fpreal spread = 1.0_f / (sigmaSqr * 2.0_f);
  const auto denom = 1.0_f / (8.0_f * std::atan(1.0_f) * sigmaSqr);

  std::vector<fpreal> gauss_x; 
  gauss_x.reserve(_dim.x);
  std::vector<fpreal> gauss_y;
  gauss_y.reserve(_dim.y);

  const auto sample = [&](const auto mid, const auto x) {
    return std::exp(-sqr(x - mid) * spread); };

  for (integer x = 0u; x < _dim.x; ++x)
  {
    gauss_x.push_back(sample(mid.x, x));
  }

  for (integer y = 0u; y < _dim.y; ++y)
  {
    gauss_y.push_back(sample(mid.y, y));
  }

  for (integer y = 0; y < _dim.y; y++) 
  { 
    for (integer x = 0; x < _dim.x; x++)
    {
      filter[y * _dim.x + x] = gauss_x[x] * gauss_y[y] * denom;
    } 
  } 

  //auto invSum = 1.0_f / std::accumulate(filter.begin(), filter.end(), 0.0_f);
  //std::transform(filter.begin(), filter.end(), filter.begin(), [&invSum]
  //    (const auto& _x)
  //    {
  //      return _x * invSum;
  //    });

  return filter;
} 

fpreal filterSum(const span<const fpreal> _filter, const uinteger2 _dimensions)
{
  return filterSum(_filter, _dimensions, _dimensions);
}

fpreal filterSum(const span<const fpreal> _filter, const uinteger2 _dimensions, const uinteger2 _crop)
{
  fpreal sum = 0.0_f;
  for (uinteger y = 0u; y < _crop.y; ++y)
  for (uinteger x = 0u; x < _crop.x; ++x)
    sum += _filter[y * _dimensions.x + x];
  return sum;
}

END_AUTOTEXGEN_NAMESPACE
