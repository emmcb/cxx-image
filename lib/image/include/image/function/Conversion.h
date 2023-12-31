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

/// Allocates a new image that is identical to the input image.
template <typename T>
Image<T> clone(const ImageView<T> &img) {
    return Image<T>(img.descriptor(), img);
}

/// Allocates a new image that is identical to the input image, but with different data type.
template <typename U, typename T, typename = std::enable_if_t<!std::is_same_v<T, U>>>
Image<U> clone(const ImageView<T> &img) {
    return Image<U>(img.descriptor(), img);
}

/// Allocates a new image and copy data with image layout conversion.
template <typename T>
Image<T> convertLayout(const ImageView<T> &img, ImageLayout imageLayout) {
    return Image<T>(LayoutDescriptor::Builder(img.layoutDescriptor()).imageLayout(imageLayout).build(), img);
}

/// Allocates a new image and copy data with image layout and pixel precision conversion.
template <typename U, typename T>
Image<U> convertLayoutAndPixelPrecision(const ImageView<T> &img, ImageLayout imageLayout, int pixelPrecision = 0) {
    ImageDescriptor<U> descriptor(ImageDescriptor<T>(LayoutDescriptor::Builder(img.layoutDescriptor())
                                                             .imageLayout(imageLayout)
                                                             .pixelPrecision(pixelPrecision)
                                                             .build(),
                                                     img.descriptor().planes));

    // int -> int conversion
    if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
        if (descriptor.saturationValue() % img.saturationValue() == 0) {
            U scale = descriptor.saturationValue() / img.saturationValue();
            return Image<U>(descriptor, img * scale);
        }

        float scale = static_cast<float>(descriptor.saturationValue()) / img.saturationValue();
        return Image<U>(descriptor, expr::lround(img * scale));
    }

    // float -> int conversion
    else if constexpr (std::is_floating_point_v<T> && std::is_integral_v<U>) {
        auto scale = descriptor.saturationValue() / img.saturationValue();
        return Image<U>(descriptor, expr::lround(img * scale));
    }

    // int -> float conversion
    else if constexpr (std::is_integral_v<T> && std::is_floating_point_v<U>) {
        auto scale = descriptor.saturationValue() / img.saturationValue();
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
    return convertLayoutAndPixelPrecision<U, T>(img, img.layoutDescriptor().imageLayout, pixelPrecision);
}

} // namespace image

} // namespace cxximg
