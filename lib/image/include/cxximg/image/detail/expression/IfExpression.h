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

/// An expression to choose between two expressions based on a condition.
template <typename IfExpr, typename ThenExpr, typename ElseExpr>
struct IfExpression final : public Expression {
    ViewType<IfExpr> ifExpr;     ///< Condition expression.
    ViewType<ThenExpr> thenExpr; ///< Then expression.
    ViewType<ElseExpr> elseExpr; ///< Else expression.

    /// Constructs expression from three expressions.
    IfExpression(IfExpr &&ifExpr_, ThenExpr &&thenExpr_, ElseExpr &&elseExpr_)
        : ifExpr(std::forward<IfExpr>(ifExpr_)),
          thenExpr(std::forward<ThenExpr>(thenExpr_)),
          elseExpr(std::forward<ElseExpr>(elseExpr_)) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(Coord... coords) const noexcept {
        return evaluate(ifExpr, coords...) ? evaluate(thenExpr, coords...) : evaluate(elseExpr, coords...);
    }
};

} // namespace detail

/// If - Then - Else expression.
template <typename IfExpr, typename ThenExpr, typename ElseExpr>
decltype(auto) iif(IfExpr &&ifExpr, ThenExpr &&thenExpr, ElseExpr &&elseExpr) {
    return detail::IfExpression<IfExpr, ThenExpr, ElseExpr>(
            std::forward<IfExpr>(ifExpr), std::forward<ThenExpr>(thenExpr), std::forward<ElseExpr>(elseExpr));
}

} // namespace expr

} // namespace cxximg
