#include "cluster.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>
#include <tuple>
#include <type_traits>
#include <gtx/fast_square_root.hpp>
#include <gtx/norm.hpp>

BEGIN_AUTOTEXGEN_NAMESPACE

namespace
{

// Calculate the smallest distance between each of the data points and any of the input means.
template <typename T, integer N, qualifier Q>
std::vector<T> findClosestDistances(
	const std::vector<vec<N, T, Q>>& _means, const span<vec<N, T, Q>>& _data) 
{
	std::vector<T> distances(_data.size());
  std::transform(_data.begin(), _data.end(), distances.begin(),
      [&](const auto& d)
      {
		    auto closest = glm::distance2(d, _means[0]);
		    for (const auto& m : _means) 
        {
          closest = glm::min(closest, glm::distance2(d, m));
		    }
        return closest;
      });
	return distances;
}

template <typename T, integer N, qualifier Q>
std::vector<vec<N, T, Q>> kmeansPlusPlusSeeds(const span<vec<N, T, Q>>& _data, uinteger _k) 
{
	std::vector<vec<N, T, Q>> means(_k);
	// Using a very simple PRBS generator, parameters selected according to
	// https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
	std::random_device rand_device;
	std::linear_congruential_engine<uint64_t, 6364136223846793005, 1442695040888963407, UINT64_MAX> rand_engine(
		rand_device());

	// Select first mean at random from the set, why not 0
	means[0] = _data[0];
	// Calculate the distance to the closest mean for each data point
	auto distances = findClosestDistances(means, _data);
  std::discrete_distribution<uinteger> generator(distances.begin(), distances.end());
  std::generate_n(means.begin() + 1, _k - 1,
      [&]()
      {
		    // Pick a random point weighted by the distance from existing means
		    // TODO: This might convert floating point weights to ints, distorting the distribution for small weights
        return _data[generator(rand_engine)];
      });

	return means;
}

/*
Calculate the index of the mean a particular data point is closest to (euclidean distance)
*/
template <typename T, integer N, qualifier Q>
uinteger findClosestMean(const vec<N, T, Q>& _point, const std::vector<vec<N, T, Q>>& _means) 
{
	T smallest_distance = glm::distance2(_point, _means[0]);
	uinteger index = 0u;
	for (uinteger i = 1u; i < _means.size(); ++i)
  {
		auto distance = glm::distance2(_point, _means[i]);
		if (distance < smallest_distance) 
    {
			smallest_distance = distance;
			index = i;
		}
	}
	return index;
}

/*
Calculate the index of the mean each data point is closest to (euclidean distance).
*/
template <typename T, integer N, qualifier Q>
std::vector<uinteger> calculateClusters(
	const span<vec<N, T, Q>>& _data, const std::vector<vec<N, T, Q>>& _means)
{
	std::vector<uinteger> clusters;
	for (auto& point : _data)
  {
		clusters.push_back(findClosestMean(point, _means));
	}
	return clusters;
}

/*
Calculate means based on data points and their cluster assignments.
*/
template <typename T, integer N, qualifier Q>
std::vector<vec<N, T, Q>> calculateMeans(
  const span<vec<N, T, Q>>& _data,
	const std::vector<uinteger>& _clusters,
	const std::vector<vec<N, T, Q>>& _old_means,
	uinteger _k)
{
	std::vector<vec<N, T, Q>> means(_k);
	std::vector<uinteger> count(_k);
  uinteger n = std::min(static_cast<uinteger>(_clusters.size()), static_cast<uinteger>(_data.size()));
	for (uinteger i = 0; i < n; ++i) 
  {
		means[_clusters[i]] += _data[i];
		++count[_clusters[i]];
	}
	for (uinteger i = 0; i < _k; ++i) 
  {
    means[i] = count[i] ? means[i] / static_cast<T>(count[i]) : _old_means[i];
	}
	return means;
}

} // namespace anonymous

std::pair<std::vector<fpreal3>, std::vector<uinteger>> kmeans_lloyd(
	const span<fpreal3> _data, uinteger _k) 
{
	std::vector<fpreal3> means = kmeansPlusPlusSeeds(_data, _k);

	std::vector<fpreal3> old_means;
	std::vector<fpreal3> old_old_means;
	std::vector<uinteger> clusters;
	// Calculate new means until convergence is reached
  // Comparison is 2*k so O(k)
  while (means != old_means && means != old_old_means)
	{
		clusters = calculateClusters(_data, means);
		old_old_means = std::move(old_means);
		old_means = means;
		means = calculateMeans(_data, clusters, old_means, _k);
	}

	return {means, clusters};
}


END_AUTOTEXGEN_NAMESPACE

