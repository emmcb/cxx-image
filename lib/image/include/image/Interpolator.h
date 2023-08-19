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

#include "image/expression/Evaluate.h"

#include "math/math.h"
#include "util/compiler.h"

namespace cxximg {

/// Nearest interpolator helper.
/// @ingroup image
struct NearestInterpolator final {
    /// Interpolates given expression at position (x, y).
    template <typename Expr, typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) interpolate(const Expr &e, float x, float y, Coord... coords) const noexcept {
        return expr::evaluate(e, std::lround(x), std::lround(y), coords...);
    }
};

/// Bilinear interpolator helper.
/// @ingroup image
struct BilinearInterpolator final {
    /// Interpolates given expression at position (x, y).
    template <typename Expr, typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) interpolate(const Expr &expr, float x, float y, Coord... coords) const noexcept {
        const int x1 = static_cast<int>(x);
        const int x2 = std::min(x1 + 1, expr.width() - 1);

        const int y1 = static_cast<int>(y);
        const int y2 = std::min(y1 + 1, expr.height() - 1);

        return math::bilinearInterpolation(evaluate(expr, x1, y1, coords...),
                                           evaluate(expr, x2, y1, coords...),
                                           evaluate(expr, x1, y2, coords...),
                                           evaluate(expr, x2, y2, coords...),
                                           x - x1,
                                           y - y1);
    }
};

/// Bicubic interpolator helper.
/// @ingroup image
struct BicubicInterpolator final {
    /// Interpolates given expression at position (x, y).
    template <typename Expr, typename... Coord>
    UTIL_ALWAYS_INLINE decltype(auto) interpolate(const Expr &expr, float x, float y, Coord... coords) const noexcept {
        const int x1 = static_cast<int>(x);
        const int x0 = std::max(x1 - 1, 0);
        const int x2 = std::min(x1 + 1, expr.width() - 1);
        const int x3 = std::min(x2 + 1, expr.width() - 1);

        const int y1 = static_cast<int>(y);
        const int y0 = std::max(y1 - 1, 0);
        const int y2 = std::min(y1 + 1, expr.height() - 1);
        const int y3 = std::min(y2 + 1, expr.height() - 1);

        return math::bicubicInterpolation(evaluate(expr, x0, y0, coords...),
                                          evaluate(expr, x1, y0, coords...),
                                          evaluate(expr, x2, y0, coords...),
                                          evaluate(expr, x3, y0, coords...),
                                          evaluate(expr, x0, y1, coords...),
                                          evaluate(expr, x1, y1, coords...),
                                          evaluate(expr, x2, y1, coords...),
                                          evaluate(expr, x3, y1, coords...),
                                          evaluate(expr, x0, y2, coords...),
                                          evaluate(expr, x1, y2, coords...),
                                          evaluate(expr, x2, y2, coords...),
                                          evaluate(expr, x3, y2, coords...),
                                          evaluate(expr, x0, y3, coords...),
                                          evaluate(expr, x1, y3, coords...),
                                          evaluate(expr, x2, y3, coords...),
                                          evaluate(expr, x3, y3, coords...),
                                          x - x1,
                                          y - y1);
    }
};

} // namespace cxximg
