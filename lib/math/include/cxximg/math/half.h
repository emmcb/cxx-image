#pragma once

#include <type_traits>

namespace cxximg {

#if defined(CXXIMG_HAVE_FLOAT16)

using half = _Float16;

// Until we support C++23, std::is_floating_point_v and std::is_arithmetic_v do not handle _Float16 type, thus we have
// to use our own values.

namespace math {

template <typename T>
inline constexpr bool is_floating_point_v = std::is_floating_point_v<T> ||
                                            std::is_same_v<_Float16, typename std::remove_cv<T>::type>;

template <typename T>
inline constexpr bool is_arithmetic_v = std::is_arithmetic_v<T> ||
                                        std::is_same_v<_Float16, typename std::remove_cv<T>::type>;

} // namespace math

#else

// Fallback to single precision float if half precision is not supported.
using half = float;

namespace math {

template <typename T>
inline constexpr bool is_floating_point_v = std::is_floating_point_v<T>;

template <typename T>
inline constexpr bool is_arithmetic_v = std::is_arithmetic_v<T>;

} // namespace math

#endif

namespace literal {

constexpr half operator""_h(long double val) {
    return static_cast<half>(val);
}

} // namespace literal

} // namespace cxximg
