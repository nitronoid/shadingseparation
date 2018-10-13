#ifndef INCLUDED_TYPES_H
#define INCLUDED_TYPES_H

#include <cstdint>
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <nonstd/span.hpp>
#include <nonstd/string_view.hpp>
#include <memory>

using fpreal = float;
using fpreal2 = glm::tvec2<fpreal>;
using fpreal3 = glm::tvec3<fpreal>;
using fpreal4 = glm::tvec4<fpreal>;

using integer = int32_t;
using int2 = glm::tvec2<integer>;
using int3 = glm::tvec3<integer>;
using int4 = glm::tvec4<integer>;

using uinteger = uint32_t;
using uint2 = glm::tvec2<uinteger>;
using uint3 = glm::tvec3<uinteger>;
using uint4 = glm::tvec4<uinteger>;

using nonstd::span;
using nonstd::string_view;

#endif //INCLUDED_TYPES_H
