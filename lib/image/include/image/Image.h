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
#include "image/expression/Expressions.h"
#include "image/function/Conversion.h"
#include "image/view/ImageView.h"

#include "util/compiler.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace cxximg {

/// @defgroup image Image storage and manipulation library
/// @copydoc libimage
/// @{

/// Image manipulation functions
/// @ingroup image
namespace image {}

/// Image storage class.
template <typename T>
class Image final : public ImageView<T> {
public:
    using ImageView<T>::operator[];
    using ImageView<T>::operator();
    using ImageView<T>::operator=;

    /// Constructs from descriptor.
    explicit Image(const ImageDescriptor<T> &imageDescriptor)
        : ImageView<T>(imageDescriptor), mSize(imageDescriptor.requiredBufferSize()), mData(new T[mSize]) {
        this->mapBuffer(mData.get());
    }

    /// Constructs from descriptor.
    template <typename U>
    explicit Image(const ImageDescriptor<U> &imageDescriptor) : Image<T>(ImageDescriptor<T>(imageDescriptor)) {}

    /// Constructs by copying an existing buffer.
    Image(const ImageDescriptor<T> &imageDescriptor, const T *buffer) : Image<T>(imageDescriptor) {
        memcpy(mData.get(), buffer, mSize * sizeof(T));
    }

    /// Constructs by evaluating an expression.
    template <typename Expr, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<Expr>>>>
    Image(const ImageDescriptor<T> &imageDescriptor, const Expr &expr) : Image<T>(imageDescriptor) {
        static_cast<ImageView<T> &>(*this) = expr;
    }

    /// Constructs by evaluating an expression.
    template <typename U, typename Expr, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<Expr>>>>
    Image(const ImageDescriptor<U> &imageDescriptor, const Expr &expr)
        : Image<T>(ImageDescriptor<T>(imageDescriptor), expr) {}

    Image(Image<T> &&) noexcept = default;

    /// Returns value at position i.
    UTIL_ALWAYS_INLINE T operator[](int64_t i) const noexcept {
        assert(i >= 0 && i < size());
        return mData[i];
    }

    /// Returns value reference at position i.
    UTIL_ALWAYS_INLINE T &operator[](int64_t i) noexcept {
        assert(i >= 0 && i < size());
        return mData[i];
    }

    /// Move assignment operator.
    Image<T> &operator=(Image<T> &&other) noexcept {
        // This operator needs a custom implementation here because the default one
        // will call ImageView<T>::operator=(ImageView<T>&&), that is already used
        // for expression assignment.

        this->mSize = std::move(other.mSize);
        this->mData = std::move(other.mData);
        this->setDescriptor(other.descriptor());

        return *this;
    }

    /// Returns raw pointer to image data.
    T *data() noexcept { return mData.get(); }

    /// Returns raw pointer to image data.
    const T *data() const noexcept { return mData.get(); }

    /// Returns image size, that is the number of values that can be stored.
    int64_t size() const noexcept { return mSize; }

    /// Re-assign image ROI.
    void setRoi(const Roi &roi) { this->setDescriptor(image::computeRoiDescriptor(this->descriptor(), roi)); }

    /// Copy data from another image.
    template <typename U>
    void copyFrom(const Image<U> &image) {
        static_cast<ImageView<T> &>(*this) = expr::cast<T>(image);
    }

    /// Allocates a new image with the same characteritics, then copy the data.
    template <typename U = T>
    Image<U> clone() const {
        return image::clone<U>(*this);
    }

private:
    int64_t mSize = 0;
    std::unique_ptr<T[]> mData;
};

using Image8i = Image<int8_t>;
using Image16i = Image<int16_t>;
using Image32i = Image<int32_t>;

using Image8u = Image<uint8_t>;
using Image16u = Image<uint16_t>;
using Image32u = Image<uint32_t>;

using Imagef = Image<float>;
using Imaged = Image<double>;

/// @}

extern template class Image<int8_t>;
extern template class Image<int16_t>;
extern template class Image<int32_t>;

extern template class Image<uint8_t>;
extern template class Image<uint16_t>;
extern template class Image<uint32_t>;

extern template class Image<float>;
extern template class Image<double>;

} // namespace cxximg
