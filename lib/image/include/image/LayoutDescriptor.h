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

#include "image/detail/Alignment.h"

#include "math/half.h"
#include "math/math.h"
#include "model/ImageLayout.h"
#include "model/PixelType.h"

#include <algorithm>
#include <array>
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

/// Structure describing plane layout.
struct PlaneDescriptor final {
    int index = 0;           ///< Plane index in image.
    int subsample = 0;       ///< Plane subsample factor, in power of two.
    int64_t offset = -1;     ///< Plane offset from start of image, in pixels.
    int64_t rowStride = 0;   ///< Distance between adjacent plane rows, in pixels.
    int64_t pixelStride = 1; ///< Distance between adjacent plane pixels, in pixels.
};

using PlaneDescriptorArray = std::array<PlaneDescriptor, image::detail::MAX_NUM_PLANES>;

/// Structure describing image layout.
/// @ingroup image
struct LayoutDescriptor final {
    class Builder;

    static const LayoutDescriptor EMPTY;

    ImageLayout imageLayout = ImageLayout::PLANAR;         ///< Image layout.
    PixelType pixelType = PixelType::CUSTOM;               ///< Pixel type.
    int pixelPrecision = 0;                                ///< Pixel precision.
    int width = 0;                                         ///< Image width in pixels.
    int height = 0;                                        ///< Image height in pixels.
    int numPlanes = 0;                                     ///< Image number of planes.
    int widthAlignment = CXXIMG_DEFAULT_WIDTH_ALIGNMENT;   ///< Width alignment (must be a power of 2).
    int heightAlignment = CXXIMG_DEFAULT_HEIGHT_ALIGNMENT; ///< Height alignment (must be a power of 2).
    int sizeAlignment = CXXIMG_DEFAULT_SIZE_ALIGNMENT;     ///< Total size alignment (must be a power of 2).
    int border = 0;                                        ///< Border around image in pixels.

    PlaneDescriptorArray planes; ///< Planes description.

    /// Compute the maximum value that can be represented by the image pixel precision.
    template <typename T>
    T saturationValue() const noexcept {
        if constexpr (math::is_floating_point_v<T>) {
            // Assumes normalized floating point images.
            return T(1);
        }

        if (pixelPrecision <= 0 || pixelPrecision > static_cast<int>(8 * sizeof(T))) {
            return std::numeric_limits<T>::max();
        }

        return (1 << pixelPrecision) - 1;
    }

    /// Compute the required buffer size in order to store image data.
    int64_t requiredBufferSize() const {
        using namespace std::string_literals;

        const int64_t bufferSize = [&] {
            const int totalHeight = height + 2 * border;

            switch (imageLayout) {
                case ImageLayout::PLANAR: {
                    return planes[0].rowStride * detail::alignDimension(totalHeight, heightAlignment) * numPlanes;
                }

                case ImageLayout::INTERLEAVED: {
                    return planes[0].rowStride * detail::alignDimension(totalHeight, heightAlignment);
                }

                case ImageLayout::YUV_420: {
                    const int64_t numPixelLumaPlane = planes[0].rowStride *
                                                      detail::alignDimension(totalHeight, heightAlignment, 0, 1);
                    const int64_t numPixelChromaPlane = planes[1].rowStride *
                                                        detail::alignDimension(totalHeight, heightAlignment, 1, 1);

                    return numPixelLumaPlane + 2 * numPixelChromaPlane;
                }

                case ImageLayout::NV12: {
                    const int64_t alignedHeight = detail::alignDimension(totalHeight, heightAlignment, 0, 1);
                    return (planes[0].rowStride + (planes[0].rowStride >> 1)) * alignedHeight;
                }

                case ImageLayout::CUSTOM: {
                    const int maxSubsample = maxSubsampleValue();

                    // Compute the amount of contiguous memory we need, taking into account planes subsample
                    int64_t bufferSize = 0;
                    for (int i = 0; i < numPlanes; ++i) {
                        bufferSize += planes[i].rowStride *
                                      detail::alignDimension(
                                              totalHeight, heightAlignment, planes[i].subsample, maxSubsample);
                    }

                    return bufferSize;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }
        }();

        return detail::alignDimension(bufferSize, sizeAlignment);
    }

    int maxSubsampleValue() const {
        const auto &plane = std::max_element(planes.begin(),
                                             planes.begin() + numPlanes,
                                             [](const auto &r, const auto &l) { return r.subsample < l.subsample; });

        return plane->subsample;
    }

private:
    LayoutDescriptor() = default;

    void updatePlanes() {
        using namespace std::string_literals;

        // Compute plane indices
        for (unsigned i = 0; i < planes.size(); ++i) {
            planes[i].index = i;
        }

        // Compute plane subsample factors
        switch (imageLayout) {
            case ImageLayout::PLANAR:
            case ImageLayout::INTERLEAVED:
                for (auto &plane : planes) {
                    plane.subsample = 0;
                }
                break;

            case ImageLayout::YUV_420:
            case ImageLayout::NV12:
                if (numPlanes != 3) {
                    throw std::invalid_argument("YUV image number of planes must be 3.");
                }

                planes[0].subsample = 0;
                planes[1].subsample = 1;
                planes[2].subsample = 1;
                break;

            case ImageLayout::CUSTOM:
                // Keep user value
                break;

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
        }

        // Compute plane strides
        const bool computeStrides = [&]() {
            for (int i = 0; i < numPlanes; ++i) {
                if (planes[0].rowStride <= 0) {
                    return true;
                }
            }
            return false; // we do not want to override user-provided strides
        }();

        if (computeStrides) {
            const int totalWidth = width + 2 * border;

            switch (imageLayout) {
                case ImageLayout::PLANAR: {
                    const int alignedWidth = detail::alignDimension(totalWidth, widthAlignment);
                    for (auto &plane : planes) {
                        plane.rowStride = alignedWidth;
                        plane.pixelStride = 1;
                    }
                    break;
                }

                case ImageLayout::INTERLEAVED: {
                    const int alignedWidth = detail::alignDimension(numPlanes * totalWidth, widthAlignment);
                    for (auto &plane : planes) {
                        plane.rowStride = alignedWidth;
                        plane.pixelStride = numPlanes;
                    }
                    break;
                }

                case ImageLayout::YUV_420: {
                    const int alignedLumaWidth = detail::alignDimension(totalWidth, widthAlignment, 0, 1);
                    const int alignedChromaWidth = detail::alignDimension(totalWidth, widthAlignment, 1, 1);

                    planes[0].rowStride = alignedLumaWidth;
                    planes[0].pixelStride = 1;

                    planes[1].rowStride = alignedChromaWidth;
                    planes[1].pixelStride = 1;

                    planes[2].rowStride = alignedChromaWidth;
                    planes[2].pixelStride = 1;
                    break;
                }

                case ImageLayout::NV12: {
                    const int alignedWidth = detail::alignDimension(totalWidth, widthAlignment, 0, 1);

                    planes[0].rowStride = alignedWidth;
                    planes[0].pixelStride = 1;

                    planes[1].rowStride = alignedWidth;
                    planes[1].pixelStride = 2;

                    planes[2].rowStride = alignedWidth;
                    planes[2].pixelStride = 2;
                    break;
                }

                case ImageLayout::CUSTOM: {
                    const int maxSubsample = maxSubsampleValue();

                    for (int i = 0; i < numPlanes; ++i) {
                        planes[i].rowStride = detail::alignDimension(
                                totalWidth, widthAlignment, planes[i].subsample, maxSubsample);
                        planes[i].pixelStride = 1;
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }
        }

        // Compute plane offsets
        const bool computeOffsets = [&]() {
            for (int i = 0; i < numPlanes; ++i) {
                if (planes[0].offset < 0) {
                    return true;
                }
            }
            return false; // we do not want to override user-provided offsets
        }();

        if (computeOffsets) {
            const int totalHeight = height + 2 * border;

            switch (imageLayout) {
                case ImageLayout::PLANAR: {
                    const int64_t numPixelPerPlane = planes[0].rowStride *
                                                     detail::alignDimension(totalHeight, heightAlignment);

                    for (unsigned i = 0; i < planes.size(); ++i) {
                        planes[i].offset = i * numPixelPerPlane;
                    }
                    break;
                }

                case ImageLayout::INTERLEAVED: {
                    for (unsigned i = 0; i < planes.size(); ++i) {
                        planes[i].offset = i;
                    }
                    break;
                }

                case ImageLayout::YUV_420: {
                    const int64_t numPixelLumaPlane = planes[0].rowStride *
                                                      detail::alignDimension(totalHeight, heightAlignment, 0, 1);
                    const int64_t numPixelChromaPlane = planes[1].rowStride *
                                                        detail::alignDimension(totalHeight, heightAlignment, 1, 1);

                    planes[0].offset = 0;
                    planes[1].offset = numPixelLumaPlane;
                    planes[2].offset = numPixelLumaPlane + numPixelChromaPlane;
                    break;
                }

                case ImageLayout::NV12: {
                    const int64_t numPixelLumaPlane = planes[0].rowStride *
                                                      detail::alignDimension(totalHeight, heightAlignment, 0, 1);

                    planes[0].offset = 0;
                    planes[1].offset = numPixelLumaPlane;
                    planes[2].offset = numPixelLumaPlane + 1;
                    break;
                }

                case ImageLayout::CUSTOM: {
                    const int maxSubsample = maxSubsampleValue();

                    int64_t offset = 0;
                    for (int i = 0; i < numPlanes; ++i) {
                        planes[i].offset = offset;

                        offset += planes[i].rowStride *
                                  detail::alignDimension(
                                          totalHeight, heightAlignment, planes[i].subsample, maxSubsample);
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }

            // Add offset for image border
            if (border > 0) {
                for (int i = 0; i < numPlanes; ++i) {
                    const int x = border >> planes[i].subsample;
                    const int y = border >> planes[i].subsample;
                    const int64_t borderOffset = y * planes[i].rowStride + x * planes[i].pixelStride;
                    planes[i].offset += borderOffset;
                }
            }
        }
    }
};

class LayoutDescriptor::Builder final {
public:
    explicit Builder(const LayoutDescriptor &descriptor) : mDescriptor(descriptor) {
        if (mDescriptor.imageLayout != ImageLayout::CUSTOM) {
            invalidatePlanes();
        }
    }

    Builder(int width, int height) noexcept {
        mDescriptor.width = width;
        mDescriptor.height = height;
    }

    Builder &imageLayout(ImageLayout imageLayout) noexcept {
        mDescriptor.imageLayout = imageLayout;
        return *this;
    }

    Builder &pixelType(PixelType pixelType) noexcept {
        mDescriptor.pixelType = pixelType;
        return *this;
    }

    Builder &pixelPrecision(int pixelPrecision) noexcept {
        mDescriptor.pixelPrecision = pixelPrecision;
        return *this;
    }

    Builder &width(int width) noexcept {
        mDescriptor.width = width;
        invalidatePlanes();
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

    Builder &heightAlignment(int heightAlignment) noexcept {
        mDescriptor.heightAlignment = heightAlignment;
        return *this;
    }

    Builder &sizeAlignment(int sizeAlignment) noexcept {
        mDescriptor.sizeAlignment = sizeAlignment;
        return *this;
    }

    Builder &border(int border) noexcept {
        mDescriptor.border = border;
        return *this;
    }

    Builder &planeSubsample(int index, int subsample) {
        mDescriptor.planes[index].subsample = subsample;
        return *this;
    }

    Builder &planeOffset(int index, int offset) {
        mDescriptor.planes[index].offset = offset;
        return *this;
    }

    Builder &planeStrides(int index, int rowStride, int pixelStride = 1) {
        mDescriptor.planes[index].rowStride = rowStride;
        mDescriptor.planes[index].pixelStride = pixelStride;
        return *this;
    }

    LayoutDescriptor build() {
        // Force YUV pixel type for YUV layouts
        if (model::isYuvLayout(mDescriptor.imageLayout)) {
            pixelType(PixelType::YUV);
        }

        // Force number of planes depending on pixel type
        const int numPlanesForPixelType = model::pixelNumPlanes(mDescriptor.pixelType);
        if (numPlanesForPixelType > 0) {
            numPlanes(numPlanesForPixelType);
        }

        // Force planar layout for grayscale and bayer pixels
        if (mDescriptor.pixelType == PixelType::GRAYSCALE || model::isBayerPixelType(mDescriptor.pixelType)) {
            imageLayout(ImageLayout::PLANAR);
        }

        // Validity checks
        if (mDescriptor.width <= 0 || mDescriptor.height <= 0 || mDescriptor.numPlanes <= 0) {
            throw std::invalid_argument("Image dimension parameters (width=" + std::to_string(mDescriptor.width) +
                                        ", height=" + std::to_string(mDescriptor.height) +
                                        ", numPlanes=" + std::to_string(mDescriptor.numPlanes) +
                                        ") must be strictly greater than zero.");
        }
        if (mDescriptor.border < 0) {
            throw std::invalid_argument("border (" + std::to_string(mDescriptor.border) +
                                        ") must be equal or greater than zero.");
        }
        if (!math::isPowerOf2(mDescriptor.widthAlignment)) {
            throw std::invalid_argument("widthAlignment (" + std::to_string(mDescriptor.widthAlignment) +
                                        ") must be a power of 2.");
        }
        if (!math::isPowerOf2(mDescriptor.heightAlignment)) {
            throw std::invalid_argument("heightAlignment (" + std::to_string(mDescriptor.heightAlignment) +
                                        ") must be a power of 2.");
        }
        if (!math::isPowerOf2(mDescriptor.sizeAlignment)) {
            throw std::invalid_argument("sizeAlignment (" + std::to_string(mDescriptor.sizeAlignment) +
                                        ") must be a power of 2.");
        }
        if (mDescriptor.numPlanes > image::detail::MAX_NUM_PLANES) {
            throw std::invalid_argument("Image number of planes (" + std::to_string(mDescriptor.numPlanes) +
                                        ") exceeds limits (" + std::to_string(image::detail::MAX_NUM_PLANES) + ").");
        }

        // Compute plane offsets and strides
        mDescriptor.updatePlanes();

        return mDescriptor;
    }

private:
    void invalidatePlanes() {
        for (unsigned i = 0; i < mDescriptor.planes.size(); ++i) {
            planeOffset(i, -1);
            planeStrides(i, 0);
        }
    }

    LayoutDescriptor mDescriptor;
};

} // namespace cxximg
