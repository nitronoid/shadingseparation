#ifndef INCLUDED_CLUSTER_H
#define INCLUDED_CLUSTER_H

#include "types.h"
#include <vector>

BEGIN_AUTOTEXGEN_NAMESPACE

// Implementation details:
// [Lloyd's Algorithm](https://en.wikipedia.org/wiki/Lloyd%27s_algorithm)
// uses [kmeans++](https://en.wikipedia.org/wiki/K-means%2B%2B)
// for initialization.
//
// @return A pair of two vectors, 
// first: a list of means, second: a list of indices that map an input to a mean

std::pair<std::vector<fpreal3>, std::vector<uinteger>> kmeans_lloyd(
	const span<fpreal3> _data, uinteger _k);

END_AUTOTEXGEN_NAMESPACE

#endif//INCLUDED_CLUSTER_H
