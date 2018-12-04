#include "normal.h"
#include <algorithm>
#include <numeric>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/matrix.hpp>
#include "glm/gtx/string_cast.hpp"

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal3> computeHeightMap(const_span<fpreal> _shading, const fpreal3 _lightDirection)
{
  const auto L = (_lightDirection);
  const uinteger numNormals = _shading.size();
  const fpreal regularization = 0.0001_f;

  std::vector<fpreal3> Nk(numNormals, {0._f, 0._f, 0._f});
  std::vector<fpreal3> Nk1(numNormals);

  fpreal d = (numNormals - 1u) * 2._f * regularization;
  fpreal3 AD(d);
  auto L2 = L * L;
  AD += L2;
  AD *= 2._f;
  auto invAD = 1._f / AD;

  auto Q = glm::outerProduct(L, L) * 2._f;
  Q[0][0] = 0._f;
  Q[1][1] = 0._f;
  Q[2][2] = 0._f;


  for (uinteger iter = 0u; iter < 100u; ++iter)
  {
    auto Nsum = std::accumulate(Nk.begin(), Nk.end(), fpreal3(0.0_f));
    for (uinteger i = 0u; i < numNormals; ++i)
    {

      fpreal3 b = 2._f * _shading[i] * L;

      // Subtract this x, y, z from total, add our scaled comps
      fpreal3 rowSum = (Nsum - Nk[i]) * -4._f *regularization + Nk[i] * Q;
      Nk1[i] = invAD * (b + rowSum);
      Nk1[i].z = std::abs(Nk1[i].z);
      Nk1[i] = glm::normalize(Nk1[i]);
    }
    std::swap(Nk1, Nk);
  }
  std::transform(Nk.begin(), Nk.end(), Nk.begin(), [](auto n) { return n * 0.5_f + 0.5_f; });
  return Nk;
}

END_AUTOTEXGEN_NAMESPACE

