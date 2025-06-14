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

#include "cxximg/image/Expressions.h"
#include "cxximg/image/ImageDescriptor.h"
#include "cxximg/image/detail/memory/AllocatorManager.h"
#include "cxximg/image/function/Conversion.h"
#include "cxximg/image/view/ImageView.h"

#include "cxximg/util/compiler.h"

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

    /// Constructs an empty image.
    Image() : ImageView<T>(LayoutDescriptor::EMPTY, nullptr) {};

    /// Constructs from layout descriptor.
    explicit Image(const LayoutDescriptor &layout) : ImageView<T>(LayoutDescriptor::Builder(layout).build(), nullptr) {
        allocate();
    }

    /// Constructs by copying an existing buffer.
    Image(const LayoutDescriptor &layout, const T *buffer) : Image<T>(layout) {
        memcpy(mData.get(), buffer, mSize * sizeof(T));
    }

    /// Constructs by evaluating an expression.
    template <typename Expr, typename = std::enable_if_t<!std::is_pointer_v<std::decay_t<Expr>>>>
    Image(const LayoutDescriptor &layout, const Expr &expr) : Image<T>(layout) {
        static_cast<ImageView<T> &>(*this) = expr;
    }

    Image(Image<T> &&) noexcept = default;

    /// Returns value at position i.
    UTIL_ALWAYS_INLINE T operator[](int64_t i) const noexcept {
        assert(i >= 0 && i < size());
        return mData[i];
    }

    /// Returns reference at position i.
    UTIL_ALWAYS_INLINE T &operator[](int64_t i) noexcept {
        assert(i >= 0 && i < size());
        return mData[i];
    }

    /// Move assignment operator.
    Image<T> &operator=(Image<T> &&other) noexcept {
        // This operator needs a custom implementation here because the default one
        // will call ImageView<T>::operator=(ImageView<T>&&), that is already used
        // for expression assignment.

        this->mSize = other.mSize;
        this->mData = std::move(other.mData);
        this->setDescriptor(other.descriptor());

        return *this;
    }

    /// Allocates the buffer for the image. Should not be called unless the image has explicitely been constructed
    /// using Image<T>::unallocated().
    void allocate() {
        if (allocated() || !defined()) {
            return;
        }

        auto &allocator = memory::detail::AllocatorManager::current();

        mSize = this->layoutDescriptor().requiredBufferSize();
        mData.reset(static_cast<T *>(allocator.allocate(mSize * sizeof(T))));
        mData.get_deleter().size = mSize;

        this->mapBuffer(mData.get());
    }

    /// Returns raw pointer to image data.
    T *data() noexcept { return mData.get(); }

    /// Returns raw pointer to image data.
    const T *data() const noexcept { return mData.get(); }

    /// Returns image size, that is the number of values that can be stored.
    int64_t size() const noexcept { return mSize; }

    /// Returns whether the image is defined.
    bool defined() const noexcept { return this->width() > 0 && this->height() > 0 && this->numPlanes() > 0; }

    /// Returns whether the image is allocated.
    bool allocated() const noexcept { return mSize > 0; }

    /// Re-assign image ROI.
    void setRoi(const Rect &roi) { this->setDescriptor(image::computeRoiDescriptor(this->descriptor(), roi)); }

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

    /// Constructs an image instance that references an already allocated image, without owning any data.
    static Image<T> borrowed(const ImageView<T> &imageView) {
        Image<T> unallocated;
        unallocated.setDescriptor(imageView.descriptor());

        return unallocated;
    }

    /// Constructs an image instance but without allocating the data.
    /// The user will be responsible for calling allocate() method before actually using the image.
    static Image<T> unallocated(const LayoutDescriptor &layout) {
        Image<T> unallocated;
        unallocated.setDescriptor(ImageDescriptor<T>(layout, nullptr));

        return unallocated;
    }

private:
    struct Deleter final {
        int64_t size = 0;

        void operator()(T *ptr) const {
            auto &allocator = memory::detail::AllocatorManager::current();
            allocator.deallocate(ptr, size * sizeof(T));
        }
    };

    int64_t mSize = 0;
    std::unique_ptr<T[], Deleter> mData;
};

using Image8i = Image<int8_t>;
using Image16i = Image<int16_t>;
using Image32i = Image<int32_t>;

using Image8u = Image<uint8_t>;
using Image16u = Image<uint16_t>;
using Image32u = Image<uint32_t>;

using Imageh = Image<half>;
using Imagef = Image<float>;
using Imaged = Image<double>;

/// @}

extern template class Image<int8_t>;
extern template class Image<int16_t>;
extern template class Image<int32_t>;

extern template class Image<uint8_t>;
extern template class Image<uint16_t>;
extern template class Image<uint32_t>;

#ifdef CXXIMG_HAVE_FLOAT16
extern template class Image<half>;
#endif
extern template class Image<float>;
extern template class Image<double>;

} // namespace cxximg
