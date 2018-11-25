#ifndef INCLUDED_TYPES_H
#define INCLUDED_TYPES_H

#include <nonstd/span.hpp>
#include <nonstd/string_view.hpp>

#include <cstdint>
#include <memory>
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>

BEGIN_AUTOTEXGEN_NAMESPACE

using glm::vec;
using glm::qualifier;

#define PPCAT_NX(A, B) A##B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIFY_NX(A) #A
#define STRINGIFY(A) STRINGIFY_NX(A)

#define DECLARE_MATH_TYPE_ALIASES(VNAME, TYPE, PRECISION)                \
  using VNAME = TYPE;                                                    \
  using PPCAT(VNAME, 2) = vec<2, VNAME, PRECISION>;                      \
  using PPCAT(VNAME, 3) = vec<3, VNAME, PRECISION>;                      \
  using PPCAT(VNAME, 4) = vec<4, VNAME, PRECISION>

DECLARE_MATH_TYPE_ALIASES(fpreal,   float,    glm::defaultp);
DECLARE_MATH_TYPE_ALIASES(integer,  int32_t,  glm::defaultp);
DECLARE_MATH_TYPE_ALIASES(uinteger, uint32_t, glm::defaultp);

#undef DECLARE_MATH_TYPE_ALIASES
#undef STRINGIFY
#undef STRINGIFY_NX
#undef PPCAT
#undef PPCAT_NX

using nonstd::span;
using nonstd::string_view;

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_TYPES_H
