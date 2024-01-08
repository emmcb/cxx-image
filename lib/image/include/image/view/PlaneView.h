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

#include "math/Histogram.h"

#include "util/compiler.h"

#include <cassert>
#include <cstdint>
#include <limits>

namespace cxximg {

/// @addtogroup image
/// @{

/// Plane manipulation class.
template <typename T>
class PlaneView final : public expr::BaseExpression {
public:
    /// Constructs plane view from specified image plane.
    PlaneView(const ImageDescriptor<T> &imageDescriptor, int index)
        : mLayoutDescriptor(imageDescriptor.layout),
          mPlaneDescriptor(imageDescriptor.planes[index]),
          mWidth((imageDescriptor.layout.width + mPlaneDescriptor.subsample) >> mPlaneDescriptor.subsample),
          mHeight((imageDescriptor.layout.height + mPlaneDescriptor.subsample) >> mPlaneDescriptor.subsample) {}

    ~PlaneView() = default;
    PlaneView(const PlaneView<T> &) noexcept = default;
    PlaneView(PlaneView<T> &&) noexcept = default;

    /// Returns value at position (x, y).
    UTIL_ALWAYS_INLINE T operator()(int x, int y) const noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mPlaneDescriptor.buffer[y * mPlaneDescriptor.rowStride + x * mPlaneDescriptor.pixelStride];
    }

    /// Returns reference at position (x, y).
    UTIL_ALWAYS_INLINE T &operator()(int x, int y) noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mPlaneDescriptor.buffer[y * mPlaneDescriptor.rowStride + x * mPlaneDescriptor.pixelStride];
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE PlaneView<T> &operator=(const PlaneView<T> &other) noexcept {
        if (this != &other) {
            operator=<PlaneView<T>>(other);
        }
        return *this;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE PlaneView<T> &operator=(PlaneView<T> &&other) noexcept {
        operator=<PlaneView<T>>(other);
        return *this;
    }

    /// Expression assignment.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PlaneView<T> &operator=(const Expr &expr) noexcept {
        forEach([&](int x, int y) UTIL_ALWAYS_INLINE { (*this)(x, y) = expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression add-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PlaneView<T> &operator+=(const Expr &expr) noexcept {
        forEach([&](int x, int y) UTIL_ALWAYS_INLINE { (*this)(x, y) += expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression subtract-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PlaneView<T> &operator-=(const Expr &expr) noexcept {
        forEach([&](int x, int y) UTIL_ALWAYS_INLINE { (*this)(x, y) -= expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression multiply-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PlaneView<T> &operator*=(const Expr &expr) noexcept {
        forEach([&](int x, int y) UTIL_ALWAYS_INLINE { (*this)(x, y) *= expr::evaluate(expr, x, y); });
        return *this;
    }

    /// Expression divide-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE PlaneView<T> &operator/=(const Expr &expr) noexcept {
        forEach([&](int x, int y) UTIL_ALWAYS_INLINE { (*this)(x, y) /= expr::evaluate(expr, x, y); });
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

    /// Returns plane descriptor.
    const PlaneDescriptor<T> &descriptor() const noexcept { return mPlaneDescriptor; }

    /// Returns layout descriptor.
    const LayoutDescriptor &layoutDescriptor() const noexcept { return mLayoutDescriptor; }

    /// Returns plane index in the underlying image.
    int index() const noexcept { return mPlaneDescriptor.index; }

    /// Returns pointer to first plane element.
    T *buffer() { return mPlaneDescriptor.buffer; }

    /// Returns pointer to first plane element.
    const T *buffer() const { return mPlaneDescriptor.buffer; }

    /// Returns pixel type.
    PixelType pixelType() const noexcept { return mLayoutDescriptor.pixelType; }

    /// Returns the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return mLayoutDescriptor.saturationValue<T>(); }

    /// Returns plane width.
    int width() const noexcept { return mWidth; }

    /// Returns plane height.
    int height() const noexcept { return mHeight; }

    /// Returns plane size, that is the number of values that can be stored.
    int size() const noexcept { return width() * height(); }

    /// Returns plane subsample factor comparing to image size, in power of two.
    int subsample() const noexcept { return mPlaneDescriptor.subsample; }

    /// Computes the plane minimum.
    T minimum() const {
        T min = std::numeric_limits<T>::max();
        forEach([&](int x, int y) {
            T pixel = (*this)(x, y);
            if (pixel < min) {
                min = pixel;
            }
        });

        return min;
    }

    /// Computes the plane maximum.
    T maximum() const {
        T max = std::numeric_limits<T>::lowest();
        forEach([&](int x, int y) {
            T pixel = (*this)(x, y);
            if (pixel > max) {
                max = pixel;
            }
        });

        return max;
    }

    /// Computes the plane mean.
    float mean() const {
        float mean = 0.0f;
        forEach([&](int x, int y) { mean += (*this)(x, y); });

        return mean / (width() * height());
    }

    /// Computes the plane histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(int numBins, T from, T to) const {
        auto histogram = hist::makeHistogram<U>(hist::RegularAxis<T>(numBins, from, to));
        forEach([&](int x, int y) { histogram((*this)(x, y)); });

        return histogram;
    }

    /// Computes the plane histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(int numBins) const {
        return histogram<U>(numBins, T(0), saturationValue());
    }

    /// Computes the plane histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(T from, T to) const {
        return histogram<U>(to - from + 1, from, to);
    }

    /// Computes the plane histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram() const {
        return histogram<U>(saturationValue() + 1, T(0), saturationValue());
    }

private:
    LayoutDescriptor mLayoutDescriptor;
    PlaneDescriptor<T> mPlaneDescriptor;
    int mWidth;
    int mHeight;
};

using PlaneView8i = PlaneView<int8_t>;
using PlaneView16i = PlaneView<int16_t>;
using PlaneView32i = PlaneView<int32_t>;

using PlaneView8u = PlaneView<uint8_t>;
using PlaneView16u = PlaneView<uint16_t>;
using PlaneView32u = PlaneView<uint32_t>;

using PlaneViewf = PlaneView<float>;
using PlaneViewd = PlaneView<double>;

/// @}

extern template class PlaneView<int8_t>;
extern template class PlaneView<int16_t>;
extern template class PlaneView<int32_t>;

extern template class PlaneView<uint8_t>;
extern template class PlaneView<uint16_t>;
extern template class PlaneView<uint32_t>;

extern template class PlaneView<float>;
extern template class PlaneView<double>;

} // namespace cxximg
