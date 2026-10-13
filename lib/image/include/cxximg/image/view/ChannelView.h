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

#include "cxximg/image/ImageDescriptor.h"
#include "cxximg/image/expression/Expression.h"

#include "cxximg/math/Histogram.h"

#include "cxximg/util/compiler.h"

#include <cassert>
#include <cstdint>
#include <limits>

namespace cxximg {

/// @addtogroup image
/// @{

/// Channel manipulation class.
template <typename T>
class ChannelView final : public expr::Expression {
public:
    /// Constructs channel view from specified image channel.
    ChannelView(const ImageDescriptor<T>& imageDescriptor, int index)
        : mLayoutDescriptor(imageDescriptor.layout),
          mChannelDescriptor(imageDescriptor.layout.channels[index]),
          mBuffer(imageDescriptor.buffer + mChannelDescriptor.offset),
          mWidth((imageDescriptor.layout.width + (1 << mChannelDescriptor.subsample) - 1) >>
                 mChannelDescriptor.subsample),
          mHeight((imageDescriptor.layout.height + (1 << mChannelDescriptor.subsample) - 1) >>
                  mChannelDescriptor.subsample),
          mDepth((imageDescriptor.layout.depth + (1 << mChannelDescriptor.subsample) - 1) >>
                 mChannelDescriptor.subsample) {}

    ~ChannelView() = default;
    ChannelView(const ChannelView<T>&) noexcept = default;
    ChannelView(ChannelView<T>&&) noexcept = default;

    /// Returns value at position (x, y).
    UTIL_ALWAYS_INLINE T operator()(int x, int y) const noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mBuffer[y * mChannelDescriptor.rowStride + x * mChannelDescriptor.pixelStride];
    }

    /// Returns reference at position (x, y).
    UTIL_ALWAYS_INLINE T& operator()(int x, int y) noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height());
        return mBuffer[y * mChannelDescriptor.rowStride + x * mChannelDescriptor.pixelStride];
    }

    /// Returns value at position (x, y, z).
    UTIL_ALWAYS_INLINE T operator()(int x, int y, int z) const noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height() && z >= 0 && z < depth());
        return mBuffer[z * mChannelDescriptor.sliceStride + y * mChannelDescriptor.rowStride +
                       x * mChannelDescriptor.pixelStride];
    }

    /// Returns reference at position (x, y, z).
    UTIL_ALWAYS_INLINE T& operator()(int x, int y, int z) noexcept {
        assert(x >= 0 && x < width() && y >= 0 && y < height() && z >= 0 && z < depth());
        return mBuffer[z * mChannelDescriptor.sliceStride + y * mChannelDescriptor.rowStride +
                       x * mChannelDescriptor.pixelStride];
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE ChannelView<T>& operator=(const ChannelView<T>& other) noexcept {
        if (this != &other) {
            operator= <ChannelView<T>>(other);
        }
        return *this;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE ChannelView<T>& operator=(ChannelView<T>&& other) noexcept {
        operator= <ChannelView<T>>(other);
        return *this;
    }

    /// Expression assignment.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ChannelView<T>& operator=(const Expr& expr) noexcept {
        forEach([&](int x, int y, int z) UTIL_ALWAYS_INLINE { (*this)(x, y, z) = expr::evaluate(expr, x, y, z); });
        return *this;
    }

    /// Expression add-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ChannelView<T>& operator+=(const Expr& expr) noexcept {
        forEach([&](int x, int y, int z) UTIL_ALWAYS_INLINE { (*this)(x, y, z) += expr::evaluate(expr, x, y, z); });
        return *this;
    }

    /// Expression subtract-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ChannelView<T>& operator-=(const Expr& expr) noexcept {
        forEach([&](int x, int y, int z) UTIL_ALWAYS_INLINE { (*this)(x, y, z) -= expr::evaluate(expr, x, y, z); });
        return *this;
    }

    /// Expression multiply-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ChannelView<T>& operator*=(const Expr& expr) noexcept {
        forEach([&](int x, int y, int z) UTIL_ALWAYS_INLINE { (*this)(x, y, z) *= expr::evaluate(expr, x, y, z); });
        return *this;
    }

    /// Expression divide-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ChannelView<T>& operator/=(const Expr& expr) noexcept {
        forEach([&](int x, int y, int z) UTIL_ALWAYS_INLINE { (*this)(x, y, z) /= expr::evaluate(expr, x, y, z); });
        return *this;
    }

    /// Invokes f(x, y, z) for each logical channel pixel, excluding borders and alignment padding.
    template <typename F>
    UTIL_ALWAYS_INLINE void forEach(F f) const noexcept {
        const int w = width();
        const int h = height();
        const int numSlices = depth();

        for (int z = 0; z < numSlices; ++z) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    f(x, y, z);
                }
            }
        }
    }

    /// Returns channel descriptor.
    const ChannelDescriptor& descriptor() const noexcept { return mChannelDescriptor; }

    /// Returns layout descriptor.
    const LayoutDescriptor& layoutDescriptor() const noexcept { return mLayoutDescriptor; }

    /// Returns channel index in the underlying image.
    int index() const noexcept { return mChannelDescriptor.index; }

    /// Returns pointer to the first channel element.
    T* buffer() const { return mBuffer; }

    /// Returns pointer to the first element of given row.
    T* buffer(int y) const { return mBuffer + y * mChannelDescriptor.rowStride; }

    /// Returns pointer to the first element of the given row in a depth slice.
    T* buffer(int y, int z) const {
        return mBuffer + z * mChannelDescriptor.sliceStride + y * mChannelDescriptor.rowStride;
    }

    /// Returns pixel type.
    PixelType pixelType() const noexcept { return mLayoutDescriptor.pixelType; }

    /// Returns the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return mLayoutDescriptor.saturationValue<T>(); }

    /// Returns channel width.
    int width() const noexcept { return mWidth; }

    /// Returns channel height.
    int height() const noexcept { return mHeight; }

    /// Returns channel depth after applying its subsampling factor.
    int depth() const noexcept { return mDepth; }

    /// Returns the number of logical channel values across all depth slices.
    int64_t size() const noexcept { return static_cast<int64_t>(width()) * height() * depth(); }

    /// Returns channel subsample factor comparing to image size, in power of two.
    int subsample() const noexcept { return mChannelDescriptor.subsample; }

    /// Computes the channel minimum.
    T minimum() const {
        T min = std::numeric_limits<T>::max();
        forEach([&](int x, int y, int z) {
            T pixel = (*this)(x, y, z);
            if (pixel < min) {
                min = pixel;
            }
        });

        return min;
    }

    /// Computes the channel maximum.
    T maximum() const {
        T max = std::numeric_limits<T>::lowest();
        forEach([&](int x, int y, int z) {
            T pixel = (*this)(x, y, z);
            if (pixel > max) {
                max = pixel;
            }
        });

        return max;
    }

    /// Computes the channel mean.
    float mean() const {
        float mean = 0.0f;
        forEach([&](int x, int y, int z) { mean += (*this)(x, y, z); });

        return mean / size();
    }

    /// Computes the channel histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(int numBins, T from, T to) const {
        auto histogram = hist::makeHistogram<U>(hist::RegularAxis<T>(numBins, from, to));
        forEach([&](int x, int y, int z) { histogram((*this)(x, y, z)); });

        return histogram;
    }

    /// Computes the channel histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(int numBins) const {
        return histogram<U>(numBins, T(0), saturationValue());
    }

    /// Computes the channel histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram(T from, T to) const {
        return histogram<U>(to - from + 1, from, to);
    }

    /// Computes the channel histogram.
    template <typename U = unsigned>
    Histogram<T, U, hist::RegularAxis> histogram() const {
        return histogram<U>(saturationValue() + 1, T(0), saturationValue());
    }

private:
    LayoutDescriptor mLayoutDescriptor;
    ChannelDescriptor mChannelDescriptor;
    T* mBuffer;
    int mWidth;
    int mHeight;
    int mDepth;
};

using ChannelView8i = ChannelView<int8_t>;
using ChannelView16i = ChannelView<int16_t>;
using ChannelView32i = ChannelView<int32_t>;

using ChannelView8u = ChannelView<uint8_t>;
using ChannelView16u = ChannelView<uint16_t>;
using ChannelView32u = ChannelView<uint32_t>;

using ChannelViewh = ChannelView<half>;
using ChannelViewf = ChannelView<float>;
using ChannelViewd = ChannelView<double>;

/// @}

extern template class ChannelView<int8_t>;
extern template class ChannelView<int16_t>;
extern template class ChannelView<int32_t>;

extern template class ChannelView<uint8_t>;
extern template class ChannelView<uint16_t>;
extern template class ChannelView<uint32_t>;

#ifdef CXXIMG_HAVE_FLOAT16
extern template class ChannelView<half>;
#endif
extern template class ChannelView<float>;
extern template class ChannelView<double>;

} // namespace cxximg
