#include "normal.h"
#include <algorithm>
#include <numeric>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/matrix.hpp>
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/vector_angle.hpp"

BEGIN_AUTOTEXGEN_NAMESPACE

std::vector<fpreal3> computeRelativeNormals(const_span<fpreal> _shading, const fpreal3 _lightDirection)
{
  const auto L = glm::normalize(_lightDirection);
  const uinteger numNormals = _shading.size();
  const fpreal regularization = 0.001_f;
  const fpreal twoLambda = 2._f * regularization;

  std::vector<fpreal3> Nk(numNormals, {0._f, 0._f, 0._f});
  std::vector<fpreal3> Nk1(numNormals);

  // Compute the self outer product of L 
  auto Q = glm::outerProduct(L, L);

  // AD is the repeating diagonal of A 
  fpreal3 aDiag((numNormals - 1u) * twoLambda);

  // Add the diagonal of Q to AD, and clear as we go
  for (uinteger i = 0u; i < 3u; ++i)
  {
    aDiag[i] += Q[i][i];
    Q[i][i] = 0.0_f;
  }
  
  // We only need the reciprocal of AD
  aDiag = 1._f / aDiag;


  for (uinteger iter = 0u; iter < 25u; ++iter)
  {
    auto Nsum = std::accumulate(Nk.begin(), Nk.end(), fpreal3(0.0_f));
    for (uinteger i = 0u; i < numNormals; ++i)
    {
      // b is 2*Si*L
      fpreal3 b = 2._f * _shading[i] * L;

      // Subtract this x, y, z from total, add our scaled comps
      fpreal3 rowSum = (Nsum - Nk[i]) * -twoLambda + Nk[i] * Q;
      // Compute the next N
      Nk1[i] = aDiag * (b + rowSum);
      // Clamp Z to positive to ensure convergence
      Nk1[i].z = std::abs(Nk1[i].z);
      // Normalize our result
      Nk1[i] = glm::normalize(Nk1[i]);
    }
    std::swap(Nk1, Nk);
  }
  return Nk;
}

fpreal solveH(const fpreal2& _N1, const fpreal2& _N2)
{
  // cc Karo 2018
  auto N1 = glm::fastNormalize(_N1);
  auto N2 = glm::fastNormalize(_N2);

  auto theta = glm::angle(N1, N2);
  auto gamma = (glm::pi<fpreal>() - theta) * 0.5_f;
  auto delta = std::abs(glm::angle({1.0_f, 0.0_f}, N1));
  delta = std::abs(std::atan(N1.y/N1.x));

  auto alpha = gamma - delta;
  auto beta  = glm::half_pi<fpreal>() - alpha;

  fpreal h = std::sin(alpha) / std::sin(beta);
  if (N1.x > 0.0 && N2.x < 0.0) h = -h;
  return h;
}

fpreal solveHG(const fpreal2& _N1, const fpreal2& _N2)
{
  auto N1 = glm::fastNormalize(_N1);
  auto N2 = glm::fastNormalize(_N2);

  auto gradient = (N1.y / N1.x);

  auto A = gradient * N2.x + N2.y;
  auto B = N2.x - gradient * N2.y;

  auto invA = 1._f / -A;
  auto disc = std::sqrt(B*B + A*A);
  auto h1 = invA * (B + disc);
  auto h2 = invA * (B - disc);

  auto beta = (h1 - gradient) / (gradient * N2.x - N2.y);
  auto alpha = (beta * N2.x + 1) / N1.x;

  if (beta * alpha > 0.0f) return h1;
  else return h2;
}


std::vector<fpreal2> computeRelativeHeights(fpreal3* _normals, uinteger2 _imageDim)
{
  const uinteger numHeights = _imageDim.x * _imageDim.y;
  std::vector<fpreal2> relativeHeights(numHeights);
  //std::cout<<solveH({-5.f, 2.f}, {-2.f, 4.f})<<'\n';

  for (uinteger y = 0u; y < _imageDim.y - 1u; ++y)
  {
    for (uinteger x = 0u; x < _imageDim.x - 1u; ++x)
    {
      auto& rh = relativeHeights[y * _imageDim.x + x];
      auto& N   = _normals[y * _imageDim.x + x];
      auto& Nnx = _normals[y * _imageDim.x + x + 1];
      auto& Nny = _normals[(y + 1u) * _imageDim.x + x];

      rh.x = solveH({N.x, N.z}, {Nnx.x, Nnx.z});
      rh.y = solveH({N.y, N.z}, {Nny.y, Nny.z});
    }
  }

  return relativeHeights;
}

std::vector<fpreal> computeAbsoluteHeights(fpreal2* _relativeHeights, uinteger2 _imageDim)
{
  const uinteger numHeights = _imageDim.x * _imageDim.y;
  std::vector<fpreal> HK(numHeights);
  std::vector<fpreal> HK1(numHeights);

  // Util for 2D indexing into 1D array
  static const auto clamp = [](int x, int lo, int hi) { return std::min(std::max(lo, x), hi); };
  auto idx = [&](auto x, auto y) { return clamp(y, 0, _imageDim.y-1) * _imageDim.x + clamp(x, 0, _imageDim.x-1); };

  for (uinteger iter = 0u; iter < 2000u; ++iter)
  {
  for (integer y = 1; y < _imageDim.y-1; ++y)
  {
    for (integer x = 1; x < _imageDim.x-1; ++x)
    {
      //h11 would be the current H
      //x offset
      auto h01 = HK[idx(x-1, y)] + _relativeHeights[idx(x-1,y)].x;
      auto h21 = HK[idx(x+1, y)] - _relativeHeights[idx(x,y)].x;
      //y offset
      auto h10 = HK[idx(x, y-1)] + _relativeHeights[idx(x,y-1)].y;
      auto h12 = HK[idx(x, y+1)] - _relativeHeights[idx(x,y)].y;

      HK1[idx(x,y)] = (h01 + h21 + h10 + h12) * 0.25_f;
    }
  }
  for (integer x = 1; x < _imageDim.x-1; ++x) HK1[idx(x,0)] = HK1[idx(x,1)];
  for (integer x = 1; x < _imageDim.x-1; ++x) HK1[idx(x,_imageDim.y-1)] = HK1[idx(x,_imageDim.y-2)];

  for (integer y = 1; y < _imageDim.y-1; ++y) HK1[idx(0, y)] = HK1[idx(1,y)];
  for (integer y = 1; y < _imageDim.y-1; ++y) HK1[idx(_imageDim.x-1, y)] = HK1[idx(_imageDim.x-2,y)] ;
  HK1[idx(0,0)] = HK1[idx(1,1)];
  HK1[idx(_imageDim.x-1,0)] = HK1[idx(_imageDim.x-2,1)];
  HK1[idx(0,_imageDim.y-1)] = HK1[idx(1,_imageDim.y-2)];
  HK1[idx(_imageDim.x-1,_imageDim.y-1)] = HK1[idx(_imageDim.x-2,_imageDim.y-2)];

  for (uinteger f = 1u; f < 2u; ++f)
  {
    //std::cout<<"iter"<<iter<<": "<<HK1[2*_imageDim.x + f]<<'\n';
  }
  std::swap(HK, HK1);
  }
  //std::transform(HK.begin(), HK.end(), HK.begin(), [&](auto h) { return -std::move(h); });
  auto max = *std::max_element(HK.begin(), HK.end());
  auto min = *std::min_element(HK.begin(), HK.end());
  auto invRange = 1._f / (max - min);
  std::cout<<"inf: "<<max<<' '<<min<<' '<<invRange<<'\n';
  std::transform(HK.begin(), HK.end(), HK.begin(), [&](auto h) { return (std::move(h) - min) * invRange; });

  return HK;

}

END_AUTOTEXGEN_NAMESPACE

