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

#include "image/Image.h"

#include <type_traits>

namespace cxximg {

namespace image {

/// Allocates an uninitialized image that has the same layout than the input image.
template <typename T>
Image<T> like(const ImageView<T> &img) {
    return Image<T>(img.layoutDescriptor());
}

/// Allocates an uninitialized image that has the same layout than the input image, but with different data type.
template <typename U, typename T, typename = std::enable_if_t<!std::is_same_v<T, U>>>
Image<U> like(const ImageView<T> &img) {
    return Image<U>(img.layoutDescriptor());
}

/// Allocates a new image that is identical to the input image.
template <typename T>
Image<T> clone(const ImageView<T> &img) {
    return Image<T>(img.layoutDescriptor(), img);
}

/// Allocates a new image that is identical to the input image, but with different data type.
template <typename U, typename T, typename = std::enable_if_t<!std::is_same_v<T, U>>>
Image<U> clone(const ImageView<T> &img) {
    return Image<U>(img.layoutDescriptor(), img);
}

/// Allocates a new image and copy data with image layout conversion.
template <typename T>
Image<T> convertLayout(const ImageView<T> &img, ImageLayout imageLayout, int widthAlignment = -1) {
    LayoutDescriptor::Builder builder(img.layoutDescriptor());
    builder.imageLayout(imageLayout);

    if (widthAlignment > 0) {
        builder.widthAlignment(widthAlignment);
    }

    return Image<T>(builder.build(), img);
}

/// Allocates a new image and copy data with image layout and pixel precision conversion.
template <typename U, typename T>
Image<U> convertPixelPrecision(const ImageView<T> &img,
                               ImageLayout imageLayout,
                               int widthAlignment = -1,
                               int pixelPrecision = 0) {
    LayoutDescriptor::Builder builder(img.layoutDescriptor());
    builder.imageLayout(imageLayout).pixelPrecision(pixelPrecision);

    if (widthAlignment > 0) {
        builder.widthAlignment(widthAlignment);
    }

    LayoutDescriptor descriptor = builder.build();

    // int -> int conversion
    if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
        if (descriptor.saturationValue<U>() % img.saturationValue() == 0) {
            U scale = descriptor.saturationValue<U>() / img.saturationValue();
            return Image<U>(descriptor, img * scale);
        }

        float scale = static_cast<float>(descriptor.saturationValue<U>()) / img.saturationValue();
        return Image<U>(descriptor, expr::lround(img * scale));
    }

    // float -> int conversion
    else if constexpr (std::is_floating_point_v<T> && std::is_integral_v<U>) {
        auto scale = descriptor.saturationValue<U>() / img.saturationValue();
        return Image<U>(descriptor, expr::lround(img * scale));
    }

    // int -> float conversion
    else if constexpr (std::is_integral_v<T> && std::is_floating_point_v<U>) {
        auto scale = descriptor.saturationValue<U>() / img.saturationValue();
        return Image<U>(descriptor, img * scale);
    }

    // float -> float conversion
    else {
        return Image<U>(descriptor, img);
    }
}

/// Allocates a new image and copy data with pixel precision conversion.
template <typename U, typename T>
Image<U> convertPixelPrecision(const ImageView<T> &img, int pixelPrecision = 0) {
    return convertPixelPrecision<U, T>(img, img.layoutDescriptor().imageLayout, pixelPrecision);
}

} // namespace image

} // namespace cxximg
