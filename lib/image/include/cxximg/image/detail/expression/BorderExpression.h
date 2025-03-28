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
#include "cxximg/image/function/BorderMode.h"

#include "cxximg/math/math.h"
#include "cxximg/util/compiler.h"

namespace cxximg {

namespace expr {

namespace detail {

/// An expression to handle borders.
template <typename Expr, BorderMode MODE>
struct BorderExpression final : public Expression {
    view_t<Expr> expr; ///< Child expression.

    /// Constructs expression from child and kernel.
    explicit BorderExpression(Expr &&expr_) : expr(std::forward<Expr>(expr_)) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(int x, int y, Coord... coords) const noexcept {
        if constexpr (MODE == BorderMode::CONSTANT) {
            if (x < 0 || x >= expr.width() || y < 0 || y >= expr.height()) {
                return 0;
            }

            return evaluate(expr, x, y, coords...);
        }

        if constexpr (MODE == BorderMode::MIRROR) {
            if (x < 0) {
                x = -x;
            } else if (x >= expr.width()) {
                x = 2 * expr.width() - x - 2;
            }

            if (y < 0) {
                y = -y;
            } else if (y >= expr.height()) {
                y = 2 * expr.height() - y - 2;
            }
        }

        if constexpr (MODE == BorderMode::NEAREST) {
            x = math::saturate(x, 0, expr.width() - 1);
            y = math::saturate(y, 0, expr.height() - 1);
        }

        if constexpr (MODE == BorderMode::REFLECT) {
            if (x < 0) {
                x = -x - 1;
            } else if (x >= expr.width()) {
                x = 2 * expr.width() - x - 1;
            }

            if (y < 0) {
                y = -y - 1;
            } else if (y >= expr.height()) {
                y = 2 * expr.height() - y - 1;
            }
        }

        return evaluate(expr, x, y, coords...);
    }
};

} // namespace detail

/// Border handling expression.
template <BorderMode MODE, typename Expr>
decltype(auto) border(Expr &&expr) {
    return detail::BorderExpression<Expr, MODE>(std::forward<Expr>(expr));
}

} // namespace expr

} // namespace cxximg
