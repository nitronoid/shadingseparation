#ifndef INCLUDED_TYPES_H
#define INCLUDED_TYPES_H

#include <nonstd/span.hpp>
#include <nonstd/string_view.hpp>

#include <cstdint>
#include <memory>
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <OpenImageIO/typedesc.h>

BEGIN_AUTOTEXGEN_NAMESPACE

using glm::vec;
using glm::qualifier;

#define PPCAT_NX(A, B) A##B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIFY_NX(A) #A
#define STRINGIFY(A) STRINGIFY_NX(A)

#define DECLARE_MATH_TYPE_ALIASES(VNAME, TYPE, PRECISION) \
  using VNAME           = TYPE;                           \
  using PPCAT(VNAME, 2) = vec<2, VNAME, PRECISION>;       \
  using PPCAT(VNAME, 3) = vec<3, VNAME, PRECISION>;       \
  using PPCAT(VNAME, 4) = vec<4, VNAME, PRECISION>

DECLARE_MATH_TYPE_ALIASES(fpreal, float, glm::defaultp);
DECLARE_MATH_TYPE_ALIASES(integer, int32_t, glm::defaultp);
DECLARE_MATH_TYPE_ALIASES(uinteger, uint32_t, glm::defaultp);

#undef DECLARE_MATH_TYPE_ALIASES
#undef STRINGIFY
#undef STRINGIFY_NX
#undef PPCAT
#undef PPCAT_NX

template <typename T>
struct TypeDescMap;

#define DECLARE_OIIO_TYPEMAP(TYPE, ENUM) \
  template<> struct TypeDescMap<TYPE> \
  { static constexpr OIIO::TypeDesc::BASETYPE type = OIIO::TypeDesc::ENUM; }

DECLARE_OIIO_TYPEMAP(float,    FLOAT);
DECLARE_OIIO_TYPEMAP(double,   DOUBLE);
DECLARE_OIIO_TYPEMAP(int8_t,   INT8);
DECLARE_OIIO_TYPEMAP(uint8_t,  UINT8);
DECLARE_OIIO_TYPEMAP(int16_t,  INT16);
DECLARE_OIIO_TYPEMAP(uint16_t, UINT16);
DECLARE_OIIO_TYPEMAP(int32_t,  INT);
DECLARE_OIIO_TYPEMAP(uint32_t, UINT);

#undef DECLARE_OIIO_TYPEMAP

constexpr fpreal operator"" _f(long double fp)
{
  return fp;
}

using nonstd::span;
using nonstd::string_view;

template <typename T, std::ptrdiff_t Extent = -1>
using const_span = const span<const T, Extent>;

END_AUTOTEXGEN_NAMESPACE

#endif  // INCLUDED_TYPES_H
