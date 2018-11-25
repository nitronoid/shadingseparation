#include "specular.h"

#include "cluster.h"
#include "morph.h"
#include "util.h"

#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <iostream>
#include <unordered_set>

BEGIN_AUTOTEXGEN_NAMESPACE

namespace
{
template <class Container, class F>
auto erase_where(Container& c, F&& f)
{
  return c.erase(std::remove_if(c.begin(), c.end(), std::forward<F>(f)),
                 c.end());
}
}  // namespace

std::vector<std::vector<uinteger>> initMaterialSets(const span<fpreal3> _albedo,
                                                    uinteger2 _imageDim,
                                                    uinteger _numSets)
{
  const uinteger numPixels = _albedo.size();
  // Use k-means clustering to group the pixels into distinct segements
  auto clusters  = kmeans_lloyd(_albedo, _numSets);
  const auto& px = std::get<1>(clusters);

  // reverse the mapping, so we have mean -> [pixels]
  std::vector<std::vector<atg::uinteger>> inv(_numSets);
  for (uinteger i = 0; i < numPixels; ++i)
  {
    inv[px[i]].push_back(i);
  }

  std::vector<std::vector<uinteger>> materialSets;
  materialSets.resize(_numSets);

  auto mask   = std::make_unique<atg::fpreal[]>(numPixels);
  auto eroded = std::make_unique<atg::fpreal[]>(numPixels);
  for (uinteger i = 0u; i < _numSets; ++i)
  {
    std::fill_n(mask.get(), numPixels, 0.0f);
    for (auto px : inv[i])
      mask[px] = 1.f;
    std::fill_n(eroded.get(), numPixels, 0.0f);
    erode(mask.get(),
          eroded.get(),
          _imageDim,
          {3, 3},
          35 * (inv[i].size() / float(numPixels)));

    auto& matSet = materialSets[i];
    for (uinteger px = 0u; px < numPixels; ++px)
    {
      if (eroded[px] > 0.0f)
        matSet.push_back(px);
    }
  }

  return materialSets;
}

namespace
{
std::vector<std::pair<uinteger, uinteger>>
closestColIndices(const span<std::vector<uinteger>> _indexSets,
                  const span<fpreal3> _cols,
                  uinteger _idx,
                  uinteger _numCols)
{
  // our albedo color
  const auto& col = _cols[_idx];
  // a vector of pairs, first: the set index, second: the index of the pixel
  std::vector<std::pair<uinteger, uinteger>> closest(_indexSets.size());
  const auto comp = [&](const auto& lhs, const auto& rhs) {
    return glm::distance2(col, _cols[lhs.second]) <
           glm::distance2(col, _cols[rhs.second]);
  };

  for (uinteger k = 0u; k < _indexSets.size(); ++k)
  {
    const auto& indices = _indexSets[k];
    for (const auto& i : indices)
    {
      std::pair<uinteger, uinteger> cur{k, i};
      if (closest.size() && comp(cur, closest[0]))
      {
        std::pop_heap(closest.begin(), closest.end(), comp);
        closest.pop_back();
      }
      if (closest.size() < _numCols)
      {
        closest.push_back(cur);
        std::push_heap(closest.begin(), closest.end(), comp);
      }
    }
  }

  return closest;
}

std::vector<uinteger> closestColIndicess(const span<const uinteger> _indexSet,
                                         const span<fpreal3> _cols,
                                         uinteger _idx,
                                         uinteger _numCols)
{
  // our albedo color
  const auto& col = _cols[_idx];
  // a vector of pairs, first: the set index, second: the index of the pixel
  std::vector<uinteger> closest(_numCols);
  const auto comp = [&](const auto& lhs, const auto& rhs) {
    return glm::distance2(col, _cols[lhs]) < glm::distance2(col, _cols[rhs]);
  };

  for (const auto& i : _indexSet)
  {
    if (closest.size() && comp(i, closest[0]))
    {
      std::pop_heap(closest.begin(), closest.end(), comp);
      closest.pop_back();
    }
    if (closest.size() < _numCols)
    {
      closest.push_back(i);
      std::push_heap(closest.begin(), closest.end(), comp);
    }
  }

  return closest;
}

}  // namespace

void removeOutliers(span<std::vector<uinteger>> _materialTypes,
                    const span<fpreal3> _albedo)
{
  const uinteger k = 10u;
  uinteger tNum    = 0u;
  std::vector<std::vector<uinteger>> materialSets(_materialTypes.size());
  for (const auto& type : _materialTypes)
  {
    for (const auto& px : type)
    {
      auto closest = closestColIndices(_materialTypes, _albedo, px, k);
      auto count =
        std::count_if(closest.begin(), closest.end(), [&](const auto& e) {
          return tNum == e.first;
        });
      if (count >= k / 2)
        materialSets[tNum].push_back(px);
    }
    ++tNum;
  }

  std::copy(materialSets.begin(), materialSets.end(), _materialTypes.begin());
}

std::vector<std::vector<fpreal>>
computeProbability(const span<std::vector<uinteger>> _materialSets,
                   const span<fpreal3> _albedo)
{
  const uinteger numPixels = _albedo.size();
  const uinteger k         = 10u;
  const fpreal ik          = 1.0f / k;
  std::vector<std::vector<fpreal>> probabilities(_materialSets.size());
  for (auto& prob : probabilities)
    prob.resize(numPixels);

  for (uinteger i = 0u; i < numPixels; ++i)
  {
    // compute set distances
    std::vector<fpreal> distances;
    distances.reserve(_materialSets.size());
    for (const auto& ms : _materialSets)
    {
      auto closest  = closestColIndicess(ms, _albedo, i, k);
      auto distance = 0.f;
      for (const auto& c : closest)
      {
        distance += glm::fastDistance(_albedo[c], _albedo[i]);
      }
      distances.push_back(1.f / (ik * distance));
    }

    auto distanceSum = std::accumulate(distances.begin(), distances.end(), 0.f);

    for (uinteger j = 0u; j < _materialSets.size(); ++j)
    {
      probabilities[j][i] = (distances[j] / distanceSum);
    }
  }

  return probabilities;
}

END_AUTOTEXGEN_NAMESPACE
