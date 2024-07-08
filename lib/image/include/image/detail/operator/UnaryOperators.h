// Copyright 2023 Emmanuel Chaboud
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

// Must be included first to make the below operators work with the Pixel class.
#include "math/Pixel.h"

#include "math/math.h"
#include "util/compiler.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace cxximg {

namespace expr {

namespace detail {

/// Absolute value operator.
struct AbsOperator final {
    template <typename T>
    UTIL_ALWAYS_INLINE T apply(T a) const noexcept {
        return std::abs(a);
    }

    template <typename T, int N>
    UTIL_ALWAYS_INLINE Pixel<T, N> apply(const Pixel<T, N> &a) const noexcept {
        Pixel<T, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Type cast operator.
template <typename T>
struct CastOperator final {
    template <typename U>
    UTIL_ALWAYS_INLINE T apply(U a) const noexcept {
        return static_cast<T>(a);
    }

    template <typename U, int N>
    UTIL_ALWAYS_INLINE Pixel<T, N> apply(const Pixel<U, N> &a) const noexcept {
        Pixel<T, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Inverse operator.
struct InvOperator final {
    template <typename T>
    UTIL_ALWAYS_INLINE float apply(T a) const noexcept {
        return 1.0f / a;
    }

    template <typename T, int N>
    UTIL_ALWAYS_INLINE Pixel<float, N> apply(const Pixel<T, N> &a) const noexcept {
        Pixel<float, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Lookup table operator.
template <typename T>
struct LutOperator final {
    const T *lut;
#ifndef NDEBUG
    int lutSize;
#endif

    template <typename U>
    UTIL_ALWAYS_INLINE T apply(U a) const noexcept {
        assert(a >= 0 && a < static_cast<int>(lutSize));
        return lut[a];
    }

    template <typename U, int N>
    UTIL_ALWAYS_INLINE Pixel<T, N> apply(const Pixel<U, N> &a) const noexcept {
        Pixel<T, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Round to int operator.
struct LRoundOperator final {
    template <typename T>
    UTIL_ALWAYS_INLINE int apply(T a) const noexcept {
        return std::lround(static_cast<float>(a));
    }

    template <typename T, int N>
    UTIL_ALWAYS_INLINE Pixel<int, N> apply(const Pixel<T, N> &a) const noexcept {
        Pixel<int, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Saturate operator.
template <typename T>
struct SaturateOperator final {
    T min;
    T max;

    template <typename U>
    UTIL_ALWAYS_INLINE U apply(U a) const noexcept {
        return math::saturate<U>(a, min, max);
    }

    template <typename U, int N>
    UTIL_ALWAYS_INLINE Pixel<U, N> apply(const Pixel<U, N> &a) const noexcept {
        Pixel<U, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Sign operator.
struct SignOperator final {
    template <typename U>
    UTIL_ALWAYS_INLINE int apply(U a) const noexcept {
        return math::sign<U>(a);
    }
};

/// Square root operator.
struct SqrtOperator final {
    template <typename T>
    UTIL_ALWAYS_INLINE float apply(T a) const noexcept {
        return std::sqrt(static_cast<float>(a));
    }

    template <typename T, int N>
    UTIL_ALWAYS_INLINE Pixel<float, N> apply(const Pixel<T, N> &a) const noexcept {
        Pixel<float, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

/// Square operator.
struct SquareOperator final {
    template <typename T>
    UTIL_ALWAYS_INLINE float apply(T a) const noexcept {
        const auto aa = static_cast<float>(a);
        return aa * aa;
    }

    template <typename T, int N>
    UTIL_ALWAYS_INLINE Pixel<float, N> apply(const Pixel<T, N> &a) const noexcept {
        Pixel<float, N> outPixel;
        for (int n = 0; n < N; ++n) {
            outPixel[n] = apply(a[n]);
        }

        return outPixel;
    }
};

} // namespace detail

} // namespace expr

} // namespace cxximg
