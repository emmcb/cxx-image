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

#include "image/detail/operator/UnaryOperators.h"
#include "image/expression/BaseExpression.h"

#include "util/compiler.h"

#include <vector>

namespace cxximg {

namespace expr {

namespace detail {

/// An unary expression, that is an unary operator applied on a child
/// expression.
template <typename Expr, class UnaryOp>
struct UnaryExpression final : public BaseExpression {
    view_t<Expr> expr; ///< Child expression.
    UnaryOp unaryOp;   ///< Unary operator to apply.

    /// Constructs expression from child.
    explicit UnaryExpression(Expr &&expr_, UnaryOp unaryOp_ = {})
        : expr(std::forward<Expr>(expr_)), unaryOp(std::move(unaryOp_)) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(Coord... coords) const noexcept {
        return unaryOp.apply(evaluate(expr, coords...));
    }
};

} // namespace detail

/// Absolute value expression.
template <typename Expr>
decltype(auto) abs(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::AbsOperator>(std::forward<Expr>(expr));
}

/// Absolute value expression.
template <typename T, typename Expr>
decltype(auto) cast(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::CastOperator<T>>(std::forward<Expr>(expr));
}

/// Conditionally round to integer if target type is integer, but do nothing if
/// target type is float.
template <typename T, typename Expr>
decltype(auto) conditionalRound(Expr &&expr) {
    if constexpr (std::is_floating_point_v<T>) {
        return std::forward<Expr>(expr);
    } else {
        return detail::UnaryExpression<Expr, detail::LRoundOperator>(std::forward<Expr>(expr));
    }
}

/// Inverse expression.
template <typename Expr>
decltype(auto) inv(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::InvOperator>(std::forward<Expr>(expr));
}

/// Lookup table expression.
template <typename T, typename Expr>
decltype(auto) lut(Expr &&expr, const std::vector<T> &lut) {
#if !defined(NDEBUG)
    return detail::UnaryExpression<Expr, detail::LutOperator<T>>(std::forward<Expr>(expr),
                                                                 {lut.data(), static_cast<int>(lut.size())});
#else
    return detail::UnaryExpression<Expr, detail::LutOperator<T>>(std::forward<Expr>(expr), {lut.data()});
#endif
}

/// Round to long expression.
template <typename Expr>
decltype(auto) lround(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::LRoundOperator>(std::forward<Expr>(expr));
}

/// Saturate expression.
template <typename T, typename Expr>
decltype(auto) saturate(Expr &&expr, T min, T max) {
    return detail::UnaryExpression<Expr, detail::SaturateOperator<T>>(std::forward<Expr>(expr), {min, max});
}

/// Sign expression.
template <typename Expr>
decltype(auto) sign(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::SignOperator>(std::forward<Expr>(expr));
}

/// Square root expression.
template <typename Expr>
decltype(auto) sqrt(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::SqrtOperator>(std::forward<Expr>(expr));
}

/// Square expression.
template <typename Expr>
decltype(auto) sq(Expr &&expr) {
    return detail::UnaryExpression<Expr, detail::SquareOperator>(std::forward<Expr>(expr));
}

} // namespace expr

} // namespace cxximg
