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

#include "cxximg/image/detail/operator/BinaryOperators.h"
#include "cxximg/image/expression/Expression.h"

#include "cxximg/util/compiler.h"

namespace cxximg {

namespace expr {

namespace detail {

/// A binary expression, that is a binary operator applied on two child
/// expressions.
template <typename LeftExpr, class BinaryOp, typename RightExpr>
struct BinaryExpression final : public Expression {
    ViewType<LeftExpr> left;   ///< Left child.
    ViewType<RightExpr> right; ///< Right child.

    /// Constructs expression from two children.
    BinaryExpression(LeftExpr &&left_, RightExpr &&right_)
        : left(std::forward<LeftExpr>(left_)), right(std::forward<RightExpr>(right_)) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(Coord... coords) const noexcept {
        return BinaryOp::apply(evaluate(left, coords...), evaluate(right, coords...));
    }
};

} // namespace detail

/// Max expression.
template <typename RightExpr, typename LeftExpr>
decltype(auto) max(LeftExpr &&left, RightExpr &&right) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::MaxOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(left), std::forward<RightExpr>(right));
}

/// Min expression.
template <typename RightExpr, typename LeftExpr>
decltype(auto) min(LeftExpr &&left, RightExpr &&right) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::MinOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(left), std::forward<RightExpr>(right));
}

/// Pow expression.
template <typename RightExpr, typename LeftExpr>
decltype(auto) pow(LeftExpr &&left, RightExpr &&right) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::PowOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(left), std::forward<RightExpr>(right));
}

} // namespace expr

} // namespace cxximg
