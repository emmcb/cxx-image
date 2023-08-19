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

// So that the below operators will work with the Pixel class.
#include "math/Pixel.h"

#include "util/compiler.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace cxximg {

namespace expr {

namespace detail {

/// Addition operator.
struct AddOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE decltype(auto) apply(T a, U b) noexcept {
        return a + b;
    }
};

/// Subtraction operator.
struct SubtractOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE decltype(auto) apply(T a, U b) noexcept {
        return a - b;
    }
};

/// Multiplication operator.
struct MultiplyOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE decltype(auto) apply(T a, U b) noexcept {
        return a * b;
    }
};

/// Division operator.
struct DivideOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE decltype(auto) apply(T a, U b) noexcept {
        return a / b;
    }
};

/// Binary left shift operator.
struct LeftShiftOperator final {
    template <typename T>
    static UTIL_ALWAYS_INLINE T apply(T a, int b) noexcept {
        return a << b;
    }
};

/// Binary right shift operator.
struct RightShiftOperator final {
    template <typename T>
    static UTIL_ALWAYS_INLINE T apply(T a, int b) noexcept {
        return a >> b;
    }
};

/// Equality operator.
struct EqualToOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a == b;
    }
};

/// Less inequality operator.
struct LessThanOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a < b;
    }
};

/// Less or equal inequality operator.
struct LessOrEqualThanOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a <= b;
    }
};

/// Greater inequality operator.
struct GreaterThanOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a > b;
    }
};

/// Greater or equal inequality operator.
struct GreaterOrEqualThanOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a >= b;
    }
};

/// Logical AND operator.
struct LogicalAndOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a && b;
    }
};

/// Logical OR operator.
struct LogicalOrOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE bool apply(T a, U b) noexcept {
        return a || b;
    }
};

/// Min operator.
struct MinOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE std::common_type_t<T, U> apply(T a, U b) noexcept {
        return std::min<std::common_type_t<T, U>>(a, b);
    }
};

/// Max operator.
struct MaxOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE std::common_type_t<T, U> apply(T a, U b) noexcept {
        return std::max<std::common_type_t<T, U>>(a, b);
    }
};

/// Power operator.
struct PowOperator final {
    template <typename T, typename U>
    static UTIL_ALWAYS_INLINE float apply(T a, U b) noexcept {
        return std::pow(static_cast<float>(a), b);
    }
};

} // namespace detail

} // namespace expr

} // namespace cxximg
