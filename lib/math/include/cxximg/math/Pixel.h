// Copyright 2023-2025 Emmanuel Chaboud
//
/// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "cxximg/math/half.h"
#include "cxximg/math/math.h"
#include "cxximg/util/compiler.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace cxximg {

/// @addtogroup math
/// @{

/// Pixel class.
template <typename T, int N>
class Pixel final {
public:
    /// Constructs empty pixel.
    constexpr Pixel() = default;

    /// Constructs pixel from scalar.
    explicit constexpr Pixel(T value) {
        for (int n = 0; n < N; ++n) {
            mPixel[n] = value;
        }
    }

    /// Constructs pixel from another pixel.
    template <typename U>
    explicit constexpr Pixel(const Pixel<U, N> &other) {
        for (int n = 0; n < N; ++n) {
            mPixel[n] = other[n];
        }
    }

    /// Constructs pixel from brace initializer.
    constexpr Pixel(const std::initializer_list<T> &initializer) {
        assert(initializer.size() == N);

        const T *data = initializer.begin();
        for (int n = 0; n < N; ++n) {
            mPixel[n] = data[n];
        }
    }

    /// Gets value at position n.
    UTIL_ALWAYS_INLINE constexpr T operator[](int n) const {
        assert(n < N);
        return mPixel[n];
    }

    /// Gets reference at position n.
    UTIL_ALWAYS_INLINE constexpr T &operator[](int n) {
        assert(n < N);
        return mPixel[n];
    }

    /// Get tuple value at position I.
    /// This allows for C++17 structured binding.
    template <std::size_t I>
    UTIL_ALWAYS_INLINE constexpr T get() const {
        static_assert(I < N);
        return mPixel[I];
    }

    /// Get tuple reference at position I.
    /// This allows for C++17 structured binding.
    template <std::size_t I>
    UTIL_ALWAYS_INLINE constexpr T &get() {
        static_assert(I < N);
        return mPixel[I];
    }

    /// Computes the minimum matrix value.
    float minimum() const { return *std::min_element(mPixel.begin(), mPixel.end()); }

    /// Computes the maximum matrix value.
    float maximum() const { return *std::max_element(mPixel.begin(), mPixel.end()); }

private:
    std::array<T, N> mPixel = {};
};

template <typename T>
using Pixel2 = Pixel<T, 2>;

template <typename T>
using Pixel3 = Pixel<T, 3>;

template <typename T>
using Pixel4 = Pixel<T, 4>;

using Pixel2u8 = Pixel<uint8_t, 2>;
using Pixel2u16 = Pixel<uint16_t, 2>;
using Pixel2h = Pixel<half, 2>;
using Pixel2f = Pixel<float, 2>;

using Pixel3u8 = Pixel<uint8_t, 3>;
using Pixel3u16 = Pixel<uint16_t, 3>;
using Pixel3h = Pixel<half, 3>;
using Pixel3f = Pixel<float, 3>;

using Pixel4u8 = Pixel<uint8_t, 4>;
using Pixel4u16 = Pixel<uint16_t, 4>;
using Pixel4h = Pixel<half, 4>;
using Pixel4f = Pixel<float, 4>;

/// @}

/// Element-wise pixel addition.
/// @relates Pixel
template <typename T, typename U, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) operator+(const Pixel<T, N> &lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] + rhs[n];
    }

    return pixel;
}

/// Addition with scalar (left side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<T>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator+(T lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs + rhs[n];
    }

    return pixel;
}

/// Addition with scalar (right side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<U>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator+(const Pixel<T, N> &lhs, U rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] + rhs;
    }

    return pixel;
}

/// Element-wise pixel subtraction.
/// @relates Pixel
template <typename T, typename U, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) operator-(const Pixel<T, N> &lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] - rhs[n];
    }

    return pixel;
}

/// Subtraction with scalar (left side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<T>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator-(T lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs - rhs[n];
    }

    return pixel;
}

/// Subtraction with scalar (right side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<U>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator-(const Pixel<T, N> &lhs, U rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] - rhs;
    }

    return pixel;
}

/// Element-wise pixel multiplication.
/// @relates Pixel
template <typename T, typename U, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) operator*(const Pixel<T, N> &lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] * rhs[n];
    }

    return pixel;
}

/// Multiplication with scalar (left side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<T>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator*(T lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs * rhs[n];
    }

    return pixel;
}

/// Multiplication with scalar (right side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<U>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator*(const Pixel<T, N> &lhs, U rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] * rhs;
    }

    return pixel;
}

/// Element-wise pixel division.
/// @relates Pixel
template <typename T, typename U, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) operator/(const Pixel<T, N> &lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] / rhs[n];
    }

    return pixel;
}

/// Division with scalar (left side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<T>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator/(T lhs, const Pixel<U, N> &rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs / rhs[n];
    }

    return pixel;
}

/// Division with scalar (right side).
/// @relates Pixel
template <typename T, typename U, int N, typename = typename std::enable_if_t<math::is_arithmetic_v<U>>>
UTIL_ALWAYS_INLINE inline decltype(auto) operator/(const Pixel<T, N> &lhs, U rhs) {
    Pixel<std::common_type_t<T, U>, N> pixel;
    for (int n = 0; n < N; ++n) {
        pixel[n] = lhs[n] / rhs;
    }

    return pixel;
}

namespace math {

/// Returns the linear interpolation between p1 and p2 at position t, where t is in [0,1].
template <typename T, int N>
UTIL_ALWAYS_INLINE inline Pixel<float, N> lerp(const Pixel<T, N> &p1, const Pixel<T, N> &p2, float t) {
    Pixel<float, N> outPixel;
    for (int n = 0; n < N; ++n) {
        outPixel[n] = lerp(p1[n], p2[n], t);
    }

    return outPixel;
}

/// Returns the bilinear interpolation between p11, p21, p12, p22 at position (tx, ty), where tx and ty are in [0,1].
template <typename T, int N>
UTIL_ALWAYS_INLINE inline Pixel<float, N> bilinearInterpolation(const Pixel<T, N> &p11,
                                                                const Pixel<T, N> &p21,
                                                                const Pixel<T, N> &p12,
                                                                const Pixel<T, N> &p22,
                                                                float tx,
                                                                float ty) {
    return lerp(lerp(p11, p21, tx), lerp(p12, p22, tx), ty);
}

} // namespace math

} // namespace cxximg

// NOLINTBEGIN
namespace std {

template <typename T, typename U, int N>
struct common_type<cxximg::Pixel<T, N>, cxximg::Pixel<U, N>> {
    using type = cxximg::Pixel<common_type_t<T, U>, N>;
};

template <typename T, int N>
struct tuple_size<cxximg::Pixel<T, N>> : integral_constant<size_t, N> {};

template <size_t I, typename T, int N>
struct tuple_element<I, cxximg::Pixel<T, N>> {
    static_assert(I < N, "Index out of bounds for Pixel");
    using type = T;
};

} // namespace std
// NOLINTEND
