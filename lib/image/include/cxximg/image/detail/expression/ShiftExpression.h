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

namespace cxximg {

namespace expr {

namespace detail {

/// An expression to shift an another expression.
template <typename Expr>
struct ShiftExpression final : public Expression {
    ViewType<Expr> expr; ///< Child expression.
    int shiftX;
    int shiftY;

    /// Constructs expression from child, shift X and shift Y.
    ShiftExpression(Expr &&expr_, int shiftX_, int shiftY_)
        : expr(std::forward<Expr>(expr_)), shiftX(shiftX_), shiftY(shiftY_) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(int x, int y, Coord... coords) const noexcept {
        return evaluate(expr, x + shiftX, y + shiftY, coords...);
    }
};

} // namespace detail

/// Shift expression.
template <typename Expr>
decltype(auto) shift(Expr &&expr, int x, int y) {
    return detail::ShiftExpression<Expr>(std::forward<Expr>(expr), x, y);
}

} // namespace expr

} // namespace cxximg
