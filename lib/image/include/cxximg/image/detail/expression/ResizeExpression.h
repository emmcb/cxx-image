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

#include "cxximg/image/Interpolator.h"
#include "cxximg/image/expression/Expression.h"

#include "cxximg/util/compiler.h"

namespace cxximg {

namespace expr {

namespace detail {

/// An expression to resize an another expression.
template <typename Expr, class Interpolator>
struct ResizeExpression final : public Expression {
    ViewType<Expr> expr; ///< Child expression.
    Interpolator interpolator;
    float offsetX;
    float offsetY;
    float scaleX;
    float scaleY;

    /// Constructs expression from child, target width, height, and optional crop rectangle.
    ResizeExpression(Expr &&expr_,
                     int width_,
                     int height_,
                     double cropX_ = 0.0,
                     double cropY_ = 0.0,
                     double cropWidth_ = 1.0,
                     double cropHeight_ = 1.0)
        : expr(std::forward<Expr>(expr_)),
          offsetX(static_cast<float>(cropX_ * (expr.width() - 1))),
          offsetY(static_cast<float>(cropY_ * (expr.height() - 1))),
          scaleX(static_cast<float>((cropWidth_ * (expr.width() - 1)) / (width_ - 1))),
          scaleY(static_cast<float>((cropHeight_ * (expr.height() - 1)) / (height_ - 1))) {}

    /// Evaluates expression at position (x, y).
    template <typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) operator()(int x, int y, Coord... coords) const noexcept {
        const float xCoord = math::saturate(offsetX + x * scaleX, 0.0f, expr.width() - 1.0f);
        const float yCoord = math::saturate(offsetY + y * scaleY, 0.0f, expr.height() - 1.0f);

        return interpolator.interpolate(expr, xCoord, yCoord, coords...);
    }
};

} // namespace detail

/// Resize expression.
template <class Interpolator, typename Expr>
decltype(auto) resize(Expr &&expr, int width, int height, bool alignCorners = true) {
    if (alignCorners) {
        return detail::ResizeExpression<Expr, Interpolator>(std::forward<Expr>(expr), width, height);
    }

    const double pixelWidth = 1.0 / (expr.width() - 1.0);
    const double pixelHeight = 1.0 / (expr.height() - 1.0);

    const double scaleX = static_cast<double>(expr.width()) / width;
    const double scaleY = static_cast<double>(expr.height()) / height;

    const double cropX = 0.5 * pixelWidth * (scaleX - 1.0);
    const double cropY = 0.5 * pixelHeight * (scaleY - 1.0);
    const double cropWidth = (width - 1.0) * pixelWidth * scaleX;
    const double cropHeight = (height - 1.0) * pixelHeight * scaleY;

    return detail::ResizeExpression<Expr, Interpolator>(
            std::forward<Expr>(expr), width, height, cropX, cropY, cropWidth, cropHeight);
}

/// Resize expression, using default bilinear interpolator.
template <typename Expr>
decltype(auto) resize(Expr &&expr, int width, int height, bool alignCorners = true) {
    return expr::resize<BilinearInterpolator>(std::forward<Expr>(expr), width, height, alignCorners);
}

/// Crop and resize expression.
template <class Interpolator, typename Expr>
decltype(auto) resize(Expr &&expr, int width, int height, float cropX, float cropY, float cropWidth, float cropHeight) {
    return detail::ResizeExpression<Expr, Interpolator>(
            std::forward<Expr>(expr), width, height, cropX, cropY, cropWidth, cropHeight);
}

/// Crop and resize expression, using default bilinear interpolator.
template <typename Expr>
decltype(auto) resize(Expr &&expr, int width, int height, float cropX, float cropY, float cropWidth, float cropHeight) {
    return detail::ResizeExpression<Expr, BilinearInterpolator>(
            std::forward<Expr>(expr), width, height, cropX, cropY, cropWidth, cropHeight);
}

} // namespace expr

} // namespace cxximg
