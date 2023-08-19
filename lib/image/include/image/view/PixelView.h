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

#include "image/ImageDescriptor.h"
#include "image/expression/BaseExpression.h"

#include "math/Pixel.h"
#include "util/compiler.h"

#include <cassert>
#include <cstdint>

namespace cxximg {

/// @addtogroup image
/// @{

/// Pixel manipulation class.
template <typename T, int N>
class PixelView final : public expr::BaseExpression {
public:
    /// Constructs image from a given descriptor.
    explicit PixelView(const ImageDescriptor<T> &imageDescriptor) : mDescriptor(imageDescriptor) {}

    ~PixelView() = default;
    PixelView(const PixelView<T, N> &) noexcept = default;
    PixelView(PixelView<T, N> &&) noexcept = default;

    /// Subset image with the given roi.
    UTIL_ALWAYS_INLINE Pixel<T, N> operator[](const Roi &roi) const {
        return Pixel<T, N>(image::computeRoiDescriptor(mDescriptor, roi));
    }

    /// Returns value at position (x, y).
    UTIL_ALWAYS_INLINE Pixel<T, N> operator()(int x, int y) const noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());

        Pixel<T, N> pixel;
        for (int n = 0; n < N; ++n) {
            const auto &planeDescriptor = mDescriptor.planes[n];
            pixel[n] = planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride];
        }

        return pixel;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator=(const PixelView<T, N> &other) noexcept {
        if (this != &other) {
            operator=<PixelView<T, N>>(other);
        }
        return *this;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator=(PixelView<T, N> &&other) noexcept {
        operator=<PixelView<T, N>>(other);
        return *this;
    }

    /// Expression assignment.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator=(const Expr &expr) noexcept {
        forEach([&](int x, int y) {
            const auto pixel = expr::evaluate(expr, x, y);
            for (int n = 0; n < N; ++n) {
                const auto &planeDescriptor = mDescriptor.planes[n];
                planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride] = pixel[n];
            }
        });
        return *this;
    }

    /// Expression add-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator+=(const Expr &expr) noexcept {
        forEach([&](int x, int y) {
            const auto pixel = expr::evaluate(expr, x, y);
            for (int n = 0; n < N; ++n) {
                const auto &planeDescriptor = mDescriptor.planes[n];
                planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride] += pixel[n];
            }
        });
        return *this;
    }

    /// Expression subtract-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator-=(const Expr &expr) noexcept {
        forEach([&](int x, int y) {
            const auto pixel = expr::evaluate(expr, x, y);
            for (int n = 0; n < N; ++n) {
                const auto &planeDescriptor = mDescriptor.planes[n];
                planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride] -= pixel[n];
            }
        });
        return *this;
    }

    /// Expression multiply-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator*=(const Expr &expr) noexcept {
        forEach([&](int x, int y) {
            const auto pixel = expr::evaluate(expr, x, y);
            for (int n = 0; n < N; ++n) {
                const auto &planeDescriptor = mDescriptor.planes[n];
                planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride] *= pixel[n];
            }
        });
        return *this;
    }

    /// Expression divide-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PixelView<T, N> &operator/=(const Expr &expr) noexcept {
        forEach([&](int x, int y) {
            const auto pixel = expr::evaluate(expr, x, y);
            for (int n = 0; n < N; ++n) {
                const auto &planeDescriptor = mDescriptor.planes[n];
                planeDescriptor.buffer[y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride] /= pixel[n];
            }
        });
        return *this;
    }

    /// Applies a function on each (x, y) coordinates.
    template <typename F>
    UTIL_ALWAYS_INLINE void forEach(F f) const noexcept {
        const int w = width();
        const int h = height();

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                f(x, y);
            }
        }
    }

    /// Returns image descriptor.
    const ImageDescriptor<T> &descriptor() const noexcept { return mDescriptor; }

    /// Returns layout descriptor.
    const LayoutDescriptor &layoutDescriptor() const noexcept { return mDescriptor.layout; }

    /// Returns image layout.
    ImageLayout imageLayout() const noexcept { return mDescriptor.layout.imageLayout; }

    /// Returns pixel type.
    PixelType pixelType() const noexcept { return mDescriptor.layout.pixelType; }

    /// Returns pixel precision.
    int pixelPrecision() const noexcept { return mDescriptor.layout.pixelPrecision; }

    /// Returns the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return mDescriptor.saturationValue(); }

    /// Returns image width.
    int width() const noexcept { return mDescriptor.layout.width; }

    /// Returns image height.
    int height() const noexcept { return mDescriptor.layout.height; }

private:
    ImageDescriptor<T> mDescriptor;
};

template <typename T>
using Pixel2View = PixelView<T, 2>;

template <typename T>
using Pixel3View = PixelView<T, 3>;

template <typename T>
using Pixel4View = PixelView<T, 4>;

using Pixel2View8u = PixelView<uint8_t, 2>;
using Pixel2View16u = PixelView<uint16_t, 2>;
using Pixel2Viewf = PixelView<float, 2>;

using Pixel3View8u = PixelView<uint8_t, 3>;
using Pixel3View16u = PixelView<uint16_t, 3>;
using Pixel3Viewf = PixelView<float, 3>;

using Pixel4View8u = PixelView<uint8_t, 4>;
using Pixel4View16u = PixelView<uint16_t, 4>;
using Pixel4Viewf = PixelView<float, 4>;

/// @}

} // namespace cxximg
