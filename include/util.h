#ifndef INCLUDED_UTIL_H
#define INCLUDED_UTIL_H

template <typename T>
struct remove_cvref 
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename C>
constexpr auto average(C&& _elems) noexcept
{
  using T = remove_cvref_t<decltype(_elems[0])>;
  auto total = static_cast<T>(0);
  for (auto&& e : _elems) total += e;
  return total / _elems.size();
}

template <typename T>
auto makeSpan(std::unique_ptr<T[]>& _data, uinteger _size) noexcept
{
  return span<T>{_data.get(), std::move(_size)};
}

#endif//INCLUDED_UTIL_H
