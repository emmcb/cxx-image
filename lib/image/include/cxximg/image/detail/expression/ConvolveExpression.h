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

#include "cxximg/image/expression/Expression.h"

#include "cxximg/util/compiler.h"

#include <array>

namespace cxximg {

namespace expr {

enum class ConvolveDirection { HORIZONTAL, VERTICAL };

namespace detail {

/// An expression to convolve with 1D kernel.
template <typename Expr, typename T, int N, ConvolveDirection DIR>
struct ConvolveExpression1D final : public Expression {
    static constexpr int HALF_KERNEL_SIZE = (N - 1) / 2;

    view_t<Expr> expr;       ///< Child expression.
    std::array<T, N> kernel; ///< kernel to convolve.

    /// Constructs expression from child and kernel.
    ConvolveExpression1D(Expr &&expr_, std::array<T, N> kernel_)
        : expr(std::forward<Expr>(expr_)), kernel(std::move(kernel_)) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(int x, int y, Coord... coords) const noexcept {
        std::common_type_t<T, decltype(evaluate(expr, x, y, coords...))> acc = 0;

        for (int i = 0; i < N; ++i) {
            if constexpr (DIR == ConvolveDirection::HORIZONTAL) {
                acc += kernel[i] * evaluate(expr, x + i - HALF_KERNEL_SIZE, y, coords...);
            } else if constexpr (DIR == ConvolveDirection::VERTICAL) {
                acc += kernel[i] * evaluate(expr, x, y + i - HALF_KERNEL_SIZE, coords...);
            }
        }

        return acc;
    }
};

} // namespace detail

/// Convolve with 1D kernel expression.
template <ConvolveDirection DIR, typename T, std::size_t N, typename Expr>
decltype(auto) convolve1d(Expr &&expr, std::array<T, N> kernel) {
    return detail::ConvolveExpression1D<Expr, T, N, DIR>(std::forward<Expr>(expr), std::move(kernel));
}

} // namespace expr

} // namespace cxximg
