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

#include "image/ImageLayout.h"
#include "image/PixelType.h"
#include "image/detail/Alignment.h"

#include "math/math.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace cxximg {

namespace image {

namespace detail {

/// Maximum number of planes an image can have.
constexpr int MAX_NUM_PLANES = 4;

} // namespace detail

} // namespace image

/// Structure describing image layout.
/// @ingroup image
struct LayoutDescriptor final {
    class Builder;

    static const LayoutDescriptor EMPTY;

    ImageLayout imageLayout = ImageLayout::PLANAR; ///< Image layout.
    PixelType pixelType = PixelType::CUSTOM;       ///< Pixel type.
    int pixelPrecision = 0;                        ///< Pixel precision.
    int width = 0;                                 ///< Image width in pixels.
    int height = 0;                                ///< Image height in pixels.
    int numPlanes = 0;                             ///< Image number of planes.
    int widthAlignment = 1;                        ///< Width alignement in power of 2.

    /// Compute the maximum value that can be represented by the image pixel precision.
    template <typename T>
    T saturationValue() const noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            // Assumes normalized floating point images.
            return T(1);
        }

        if (pixelPrecision <= 0 || pixelPrecision > static_cast<int>(8 * sizeof(T))) {
            return std::numeric_limits<T>::max();
        }

        return (1 << pixelPrecision) - 1;
    }

    /// Compute required buffer size in order to store pixels.
    int64_t requiredBufferSize() const {
        using namespace std::string_literals;

        switch (imageLayout) {
            case ImageLayout::PLANAR:
                return static_cast<int64_t>(numPlanes) * detail::alignDimension(width, widthAlignment) * height;

            case ImageLayout::INTERLEAVED:
                return static_cast<int64_t>(detail::alignDimension(numPlanes * width, widthAlignment)) * height;

            case ImageLayout::YUV_420: {
                if (numPlanes != 3) {
                    throw std::invalid_argument("YUV_420 image number of planes (" + std::to_string(numPlanes) +
                                                ") must be 3.");
                }
                const int alignedLumaWidth = detail::alignDimension(width, widthAlignment, 0, 1);
                const int alignedChromaWidth = detail::alignDimension(width, widthAlignment, 1, 1);
                const int alignedHeight = detail::alignDimension(height, 1, 0, 1);
                return static_cast<int64_t>(alignedLumaWidth + alignedChromaWidth) * alignedHeight;
            }

            case ImageLayout::NV12: {
                if (numPlanes != 3) {
                    throw std::invalid_argument("NV12 image number of planes (" + std::to_string(numPlanes) +
                                                ") must be 3.");
                }
                const int alignedWidth = detail::alignDimension(width, widthAlignment, 0, 1);
                const int alignedHeight = detail::alignDimension(height, 1, 0, 1);
                return static_cast<int64_t>(alignedWidth + (alignedWidth >> 1)) * alignedHeight;
            }

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
        }
    }

private:
    LayoutDescriptor() = default;
};

class LayoutDescriptor::Builder final {
public:
    explicit Builder(const LayoutDescriptor &descriptor) : mDescriptor(descriptor) {}

    Builder(int width, int height) noexcept {
        mDescriptor.width = width;
        mDescriptor.height = height;
    }

    Builder &imageLayout(ImageLayout imageLayout) noexcept {
        mDescriptor.imageLayout = imageLayout;

        // Force YUV pixel type for YUV layouts
        if (image::isYuvLayout(imageLayout)) {
            pixelType(PixelType::YUV);
        }

        return *this;
    }

    Builder &pixelType(PixelType pixelType) noexcept {
        mDescriptor.pixelType = pixelType;

        // Force number of planes depending on pixel type
        const int numPlanesForPixelType = image::pixelNumPlanes(pixelType);
        if (numPlanesForPixelType > 0) {
            numPlanes(numPlanesForPixelType);
        }

        // Force planar layout for grayscale and bayer pixels
        if (pixelType == PixelType::GRAYSCALE || image::isBayerPixelType(pixelType)) {
            imageLayout(ImageLayout::PLANAR);
        }

        return *this;
    }

    Builder &pixelPrecision(int pixelPrecision) noexcept {
        mDescriptor.pixelPrecision = pixelPrecision;
        return *this;
    }

    Builder &width(int width) noexcept {
        mDescriptor.width = width;
        return *this;
    }

    Builder &height(int height) noexcept {
        mDescriptor.height = height;
        return *this;
    }

    Builder &numPlanes(int numPlanes) noexcept {
        mDescriptor.numPlanes = numPlanes;
        return *this;
    }

    Builder &widthAlignment(int widthAlignment) noexcept {
        mDescriptor.widthAlignment = widthAlignment;
        return *this;
    }

    LayoutDescriptor build() {
        if (mDescriptor.width <= 0 || mDescriptor.height <= 0 || mDescriptor.numPlanes <= 0) {
            throw std::invalid_argument("Image dimension parameters (width=" + std::to_string(mDescriptor.width) +
                                        ", height=" + std::to_string(mDescriptor.height) +
                                        ", numPlanes=" + std::to_string(mDescriptor.numPlanes) +
                                        ") must be strictly greater than zero.");
        }
        if (!math::isPowerOf2(mDescriptor.widthAlignment)) {
            throw std::invalid_argument("widthAlignment (" + std::to_string(mDescriptor.widthAlignment) +
                                        ") must be a power of 2.");
        }
        if (mDescriptor.numPlanes > image::detail::MAX_NUM_PLANES) {
            throw std::invalid_argument("Image number of planes (" + std::to_string(mDescriptor.numPlanes) +
                                        ") exceeds limits (" + std::to_string(image::detail::MAX_NUM_PLANES) + ").");
        }

        return mDescriptor;
    }

private:
    LayoutDescriptor mDescriptor;
};

} // namespace cxximg
