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

#include "image/expression/BaseExpression.h"
#include "image/view/PlaneView.h"

#include "util/compiler.h"

#include <cassert>
#include <cstdint>
#include <utility>

namespace cxximg {

/// @addtogroup image
/// @{

/// Bayer manipulation class.
template <typename T>
class BayerView final : public expr::BaseExpression {
public:
    /// Constructs bayer view from specified bayer image and bayer color.
    BayerView(PlaneView<T> rawView, Bayer bayer)
        : mRawView(std::move(rawView)),
          mXOffset(image::bayerXOffset(rawView.pixelType(), bayer)),
          mYOffset(image::bayerYOffset(rawView.pixelType(), bayer)) {}

    /// Constructs bayer view from specified bayer image and bayer color.
    BayerView(PlaneView<T> rawView, PixelType pixelType, Bayer bayer)
        : mRawView(std::move(rawView)),
          mXOffset(image::bayerXOffset(pixelType, bayer)),
          mYOffset(image::bayerYOffset(pixelType, bayer)) {}

    ~BayerView() = default;
    BayerView(const BayerView<T> &) noexcept = default;
    BayerView(BayerView<T> &&) noexcept = default;

    /// Returns value at position (x, y).
    UTIL_ALWAYS_INLINE T operator()(int x, int y) const noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mRawView(2 * x + mXOffset, 2 * y + mYOffset);
    }

    /// Returns reference at position (x, y).
    UTIL_ALWAYS_INLINE T &operator()(int x, int y) noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mRawView(2 * x + mXOffset, 2 * y + mYOffset);
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE BayerView<T> &operator=(const BayerView<T> &other) noexcept {
        if (this != &other) {
            operator=<BayerView<T>>(other);
        }
        return *this;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE BayerView<T> &operator=(BayerView<T> &&other) noexcept {
        operator=<BayerView<T>>(other);
        return *this;
    }

    /// Expression assignment.
    template <typename Expr>
    UTIL_ALWAYS_INLINE BayerView<T> &operator=(const Expr &expr) noexcept {
        forEach([&](int x, int y) { mRawView(x, y) = expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression add-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE BayerView<T> &operator+=(const Expr &expr) noexcept {
        forEach([&](int x, int y) { mRawView(x, y) += expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression subtract-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE BayerView<T> &operator-=(const Expr &expr) noexcept {
        forEach([&](int x, int y) { mRawView(x, y) -= expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression multiply-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE BayerView<T> &operator*=(const Expr &expr) noexcept {
        forEach([&](int x, int y) { mRawView(x, y) *= expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression divide-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE BayerView<T> &operator/=(const Expr &expr) noexcept {
        forEach([&](int x, int y) { mRawView(x, y) /= expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Applies a function on each (x, y) coordinates.
    template <typename F>
    UTIL_ALWAYS_INLINE void forEach(F f) const noexcept {
        const int w = width() * 2;
        const int h = height() * 2;

        for (int y = mYOffset; y < h; y += 2) {
            for (int x = mXOffset; x < w; x += 2) {
                f(x, y);
            }
        }
    }

    /// Returns plane width.
    int width() const noexcept { return mRawView.width() / 2; }

    /// Returns plane height.
    int height() const noexcept { return mRawView.height() / 2; }

private:
    PlaneView<T> mRawView;
    int mXOffset;
    int mYOffset;
};

using BayerView8i = BayerView<int8_t>;
using BayerView16i = BayerView<int16_t>;
using BayerView32i = BayerView<int32_t>;

using BayerView8u = BayerView<uint8_t>;
using BayerView16u = BayerView<uint16_t>;
using BayerView32u = BayerView<uint16_t>;

using BayerViewf = BayerView<float>;
using BayerViewd = BayerView<double>;

/// @}

} // namespace cxximg
