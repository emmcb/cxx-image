#pragma once

namespace cxximg {

#if defined(HAVE_FLOAT16)
using half_t = _Float16;
#else
using half_t = float;
#endif

namespace literal {

constexpr half_t operator""_h(long double val) {
    return static_cast<half_t>(val);
}

} // namespace literal

} // namespace cxximg
