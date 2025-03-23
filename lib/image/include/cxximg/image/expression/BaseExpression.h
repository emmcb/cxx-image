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
#include "cxximg/image/expression/Evaluate.h"
#include "cxximg/image/expression/View.h"

#include <type_traits>
#include <utility>

namespace cxximg {

namespace expr {

namespace detail {

template <typename, class, typename>
struct BinaryExpression;

} // namespace detail

/// Expression base class that provides basic operators (+, -, *, /).
struct BaseExpression {};

template <typename LeftExpr, typename RightExpr>
using IsBaseExpression = std::disjunction<std::is_base_of<BaseExpression, std::remove_reference_t<LeftExpr>>,
                                          std::is_base_of<BaseExpression, std::remove_reference_t<RightExpr>>>;

/// Expression addition.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator+(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::AddOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression subtraction.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator-(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::SubtractOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression multiplication.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator*(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::MultiplyOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression division.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator/(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::DivideOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression binary left shift.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator<<(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::LeftShiftOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression binary right shift.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator>>(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::RightShiftOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression comparison.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator==(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::EqualToOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression comparison.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator<(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::LessThanOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression comparison.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator<=(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::LessOrEqualThanOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression comparison.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator>(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::GreaterThanOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression comparison.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator>=(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::GreaterOrEqualThanOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression logical AND.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator&&(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::LogicalAndOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

/// Expression logical OR.
/// @relates BaseExpression
template <typename LeftExpr,
          typename RightExpr,
          typename = std::enable_if_t<IsBaseExpression<LeftExpr, RightExpr>::value>>
decltype(auto) operator||(LeftExpr &&leftExpr, RightExpr &&rightExpr) {
    using BinaryExpression = detail::BinaryExpression<LeftExpr, detail::LogicalOrOperator, RightExpr>;
    return BinaryExpression(std::forward<LeftExpr>(leftExpr), std::forward<RightExpr>(rightExpr));
}

} // namespace expr

} // namespace cxximg
