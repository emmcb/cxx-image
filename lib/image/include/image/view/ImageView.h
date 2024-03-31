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
#include "image/view/PlaneView.h"

#include "util/compiler.h"

#include <cassert>
#include <cstdint>
#include <limits>

namespace cxximg {

/// @addtogroup image
/// @{

/// Image manipulation class.
template <typename T>
class ImageView : public expr::BaseExpression {
public:
    class PlaneIterable;

    /// Constructs image from a given descriptor.
    ImageView(const ImageDescriptor<T> &imageDescriptor) // NOLINT(google-explicit-constructor)
        : mDescriptor(imageDescriptor) {}

    /// Constructs one-plane image view from plane view.
    ImageView(const PlaneView<T> &planeView) // NOLINT(google-explicit-constructor)
        : ImageView(ImageDescriptor<T>(
                  LayoutDescriptor::Builder(planeView.width(), planeView.height())
                          .numPlanes(1)
                          .imageLayout(ImageLayout::CUSTOM)
                          .planeStrides(0, planeView.descriptor().rowStride, planeView.descriptor().pixelStride)
                          .build(),
                  {planeView.buffer()})) {}

    ~ImageView() = default;
    ImageView(const ImageView<T> &) noexcept = default;
    ImageView(ImageView<T> &&) noexcept = default;

    /// Returns corresponding plane to an other image plane.
    template <typename U>
    UTIL_ALWAYS_INLINE PlaneView<T> operator[](const PlaneView<U> &plane) const {
        assert(plane.index() >= 0 && plane.index() < numPlanes());
        return PlaneView<T>(mDescriptor, plane.index());
    }

    /// Subset image with the given roi.
    UTIL_ALWAYS_INLINE ImageView<T> operator[](const Roi &roi) const {
        return ImageView<T>(image::computeRoiDescriptor(mDescriptor, roi));
    }

    /// Returns value at position (x, y, n).
    UTIL_ALWAYS_INLINE T operator()(int x, int y, int n) const noexcept {
        // assert(n >= 0 && n < numPlanes() && x >= 0 && x < plane(n).width() && y >= 0 && y < plane(n).height());

        const auto &planeDescriptor = mDescriptor.layout.planes[n];
        return mDescriptor.buffers[n][y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride];
    }

    /// Returns reference at position (x, y, n).
    UTIL_ALWAYS_INLINE T &operator()(int x, int y, int n) noexcept {
        // assert(n >= 0 && n < numPlanes() && x >= 0 && x < plane(n).width() && y >= 0 && y < plane(n).height());

        const auto &planeDescriptor = mDescriptor.layout.planes[n];
        return mDescriptor.buffers[n][y * planeDescriptor.rowStride + x * planeDescriptor.pixelStride];
    }

#ifdef HAVE_HALIDE
    operator halide_buffer_t *() const { // NOLINT(google-explicit-constructor)
        return &mDescriptor.halide->buffer;
    }
#endif

    /// Expression assignment.
    UTIL_ALWAYS_INLINE ImageView<T> &operator=(const ImageView<T> &other) noexcept {
        if (this != &other) {
            operator= <ImageView<T>>(other);
        }
        return *this;
    }

    /// Expression assignment.
    UTIL_ALWAYS_INLINE ImageView<T> &operator=(ImageView<T> &&other) noexcept {
        operator= <ImageView<T>>(other);
        return *this;
    }

    /// Expression assignment.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ImageView<T> &operator=(const Expr &expr) noexcept {
        forEach([&](int x, int y, int n) UTIL_ALWAYS_INLINE { (*this)(x, y, n) = expr::evaluate(expr, x, y, n); });
        return *this;
    }

    /// Expression add-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ImageView<T> &operator+=(const Expr &expr) noexcept {
        forEach([&](int x, int y, int n) UTIL_ALWAYS_INLINE { (*this)(x, y, n) += expr::evaluate(expr, x, y, n); });
        return *this;
    }

    /// Expression subtract-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ImageView<T> &operator-=(const Expr &expr) noexcept {
        forEach([&](int x, int y, int n) UTIL_ALWAYS_INLINE { (*this)(x, y, n) -= expr::evaluate(expr, x, y, n); });
        return *this;
    }

    /// Expression multiply-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ImageView<T> &operator*=(const Expr &expr) noexcept {
        forEach([&](int x, int y, int n) UTIL_ALWAYS_INLINE { (*this)(x, y, n) *= expr::evaluate(expr, x, y, n); });
        return *this;
    }

    /// Expression divide-assign.
    template <typename Expr>
    UTIL_ALWAYS_INLINE ImageView<T> &operator/=(const Expr &expr) noexcept {
        forEach([&](int x, int y, int n) UTIL_ALWAYS_INLINE { (*this)(x, y, n) /= expr::evaluate(expr, x, y, n); });
        return *this;
    }

    /// Applies a function on each (x, y) coordinates.
    template <typename F>
    UTIL_ALWAYS_INLINE void forEach(F f) const noexcept {
        const int dim = numPlanes();

        for (int n = 0; n < dim; ++n) {
            const int subsample = mDescriptor.layout.planes[n].subsample;
            const int w = (width() + subsample) >> subsample;
            const int h = (height() + subsample) >> subsample;

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    f(x, y, n);
                }
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

    /// Sets descriptor pixel precision.
    /// @warning This method does not rescale the data. See
    /// image::convertPixelPrecision().
    void setPixelPrecision(int pixelPrecision) noexcept { mDescriptor.layout.pixelPrecision = pixelPrecision; }

    /// Returns the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return mDescriptor.saturationValue(); }

    /// Returns image width.
    int width() const noexcept { return mDescriptor.layout.width; }

    /// Returns image height.
    int height() const noexcept { return mDescriptor.layout.height; }

    /// Returns image number of planes.
    int numPlanes() const noexcept { return mDescriptor.layout.numPlanes; }

    /// Returns an iterable object to image planes.
    PlaneIterable planes() const noexcept { return PlaneIterable(mDescriptor); }

    /// Returns the given plane.
    PlaneView<T> plane(int index) const {
        assert(index < numPlanes());
        return PlaneView<T>{mDescriptor, index};
    }

    /// Computes the image minimum.
    T minimum() const {
        T min = std::numeric_limits<T>::max();

        for (auto plane : planes()) {
            T planeMin = plane.minimum();
            if (planeMin < min) {
                min = planeMin;
            }
        }

        return min;
    }

    /// Computes the image maximum.
    T maximum() const {
        T max = std::numeric_limits<T>::lowest();

        for (auto plane : planes()) {
            T planeMax = plane.maximum();
            if (planeMax > max) {
                max = planeMax;
            }
        }

        return max;
    }

protected:
    /// Set view descriptor.
    void setDescriptor(const ImageDescriptor<T> &descriptor) noexcept { mDescriptor = descriptor; }

    /// Map view descriptor to another buffer.
    void mapBuffer(T *buffer) { mDescriptor.map(buffer); }

private:
    ImageDescriptor<T> mDescriptor;
};

template <typename T>
class ImageView<T>::PlaneIterable final {
public:
    class Iterator;

    /// Constructor.
    explicit PlaneIterable(const ImageDescriptor<T> &imageDescriptor) : mDescriptor(imageDescriptor) {}

    /// Returns plane at position i.
    PlaneView<T> operator[](int i) const { return PlaneView<T>(mDescriptor, i); }

    /// Returns iterator to first plane in image.
    Iterator begin() const { return Iterator(mDescriptor); }

    /// Returns iterator to past-last plane in image.
    Iterator end() const { return Iterator(mDescriptor, mDescriptor.layout.numPlanes); }

private:
    ImageDescriptor<T> mDescriptor;
};

template <typename T>
class ImageView<T>::PlaneIterable::Iterator final {
public:
    /// Prefix increment.
    Iterator &operator++() noexcept {
        ++mIndex;
        return *this;
    }

    /// Equality test.
    bool operator==(const Iterator &iterator) const noexcept { return mIndex == iterator.mIndex; }
    bool operator!=(const Iterator &iterator) const noexcept { return !operator==(iterator); }

    /// Gets the value.
    PlaneView<T> operator*() const { return PlaneView<T>(mDescriptor, mIndex); }

private:
    friend class PlaneIterable;

    /// Constructor.
    explicit Iterator(const ImageDescriptor<T> &imageDescriptor, int index = 0)
        : mDescriptor(imageDescriptor), mIndex(index) {}

    ImageDescriptor<T> mDescriptor;
    int mIndex;
};

using ImageView8i = ImageView<int8_t>;
using ImageView16i = ImageView<int16_t>;
using ImageView32i = ImageView<int32_t>;

using ImageView8u = ImageView<uint8_t>;
using ImageView16u = ImageView<uint16_t>;
using ImageView32u = ImageView<uint32_t>;

using ImageViewf = ImageView<float>;
using ImageViewd = ImageView<double>;

/// @}

extern template class ImageView<int8_t>;
extern template class ImageView<int16_t>;
extern template class ImageView<int32_t>;

extern template class ImageView<uint8_t>;
extern template class ImageView<uint16_t>;
extern template class ImageView<uint32_t>;

extern template class ImageView<float>;
extern template class ImageView<double>;

} // namespace cxximg
