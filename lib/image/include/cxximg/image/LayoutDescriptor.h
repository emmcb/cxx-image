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

#include "cxximg/image/detail/Alignment.h"

#include "cxximg/math/half.h"
#include "cxximg/math/math.h"
#include "cxximg/model/ImageLayout.h"
#include "cxximg/model/PixelType.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace cxximg {

namespace image {

namespace detail {

/// Maximum number of channels an image can have.
constexpr int MAX_NUM_CHANNELS = 4;

} // namespace detail

} // namespace image

/// Structure describing channel layout.
struct ChannelDescriptor final {
    int index = 0;           ///< Channel index in image.
    int subsample = 0;       ///< Channel XYZ subsample factor, in power of two.
    int64_t offset = -1;     ///< Channel offset from start of image, in pixels.
    int64_t sliceStride = 0; ///< Distance between adjacent channel depth slices, in pixels.
    int64_t rowStride = 0;   ///< Distance between adjacent channel rows, in pixels.
    int64_t pixelStride = 1; ///< Distance between adjacent channel pixels, in pixels.
};

using ChannelDescriptorArray = std::array<ChannelDescriptor, image::detail::MAX_NUM_CHANNELS>;

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
    int depth = 1;                                         ///< Image depth in slices, without border or subsampling.
    int numChannels = 0;                                   ///< Image number of channels.
    int widthAlignment = CXXIMG_DEFAULT_WIDTH_ALIGNMENT;   ///< Width alignment (must be a power of 2).
    int heightAlignment = CXXIMG_DEFAULT_HEIGHT_ALIGNMENT; ///< Height alignment (must be a power of 2).
    int depthAlignment = CXXIMG_DEFAULT_DEPTH_ALIGNMENT;   ///< Depth alignment (must be a power of 2).
    int sizeAlignment = CXXIMG_DEFAULT_SIZE_ALIGNMENT;     ///< Total size alignment (must be a power of 2).
    int border = 0;                                        ///< Border around image in pixels.

    ChannelDescriptorArray channels; ///< Channels description.

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
            switch (imageLayout) {
                case ImageLayout::PLANAR: {
                    return channelBufferSize(0) * numChannels;
                }

                case ImageLayout::INTERLEAVED: {
                    return channelBufferSize(0);
                }

                case ImageLayout::YUV_420: {
                    return channelBufferSize(0) + 2 * channelBufferSize(1);
                }

                case ImageLayout::NV12: {
                    return channelBufferSize(0) + channelBufferSize(1);
                }

                case ImageLayout::CUSTOM: {
                    // Each custom channel stores all its depth slices contiguously.
                    int64_t bufferSize = 0;
                    for (int i = 0; i < numChannels; ++i) {
                        bufferSize += channelBufferSize(i);
                    }

                    return bufferSize;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }
        }();

        return detail::alignDimension(bufferSize, sizeAlignment);
    }

    /// Returns the largest spatial subsampling factor among the image channels.
    int maxSubsampleValue() const {
        const auto& channel = std::max_element(channels.begin(),
                                               channels.begin() + numChannels,
                                               [](const auto& r, const auto& l) { return r.subsample < l.subsample; });

        return channel->subsample;
    }

private:
    LayoutDescriptor() = default;

    /// Computes a channel's storage span, including slice padding and aligned subsampled depth.
    int64_t channelBufferSize(int index) const {
        const int subsample = channels[index].subsample;
        const int subsampledDepth = (depth + (1 << subsample) - 1) >> subsample;
        const int alignedDepth = detail::alignDimension(subsampledDepth, depthAlignment);
        return channels[index].sliceStride * alignedDepth;
    }

    /// Computes missing channel strides and offsets from the image dimensions and layout.
    void updateChannels() {
        using namespace std::string_literals;

        // Compute channel indices
        for (unsigned i = 0; i < channels.size(); ++i) {
            channels[i].index = i;
        }

        // Compute channel subsample factors
        switch (imageLayout) {
            case ImageLayout::PLANAR:
            case ImageLayout::INTERLEAVED:
                for (auto& channel : channels) {
                    channel.subsample = 0;
                }
                break;

            case ImageLayout::YUV_420:
            case ImageLayout::NV12:
                if (numChannels != 3) {
                    throw std::invalid_argument("YUV image number of channels must be 3.");
                }

                channels[0].subsample = 0;
                channels[1].subsample = 1;
                channels[2].subsample = 1;
                break;

            case ImageLayout::CUSTOM:
                // Keep user value
                break;

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
        }

        // Compute channel strides
        const bool computeStrides = [&]() {
            for (int i = 0; i < numChannels; ++i) {
                if (channels[0].rowStride <= 0) {
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
                    for (auto& channel : channels) {
                        channel.rowStride = alignedWidth;
                        channel.pixelStride = 1;
                    }
                    break;
                }

                case ImageLayout::INTERLEAVED: {
                    const int alignedWidth = detail::alignDimension(numChannels * totalWidth, widthAlignment);
                    for (auto& channel : channels) {
                        channel.rowStride = alignedWidth;
                        channel.pixelStride = numChannels;
                    }
                    break;
                }

                case ImageLayout::YUV_420: {
                    const int alignedLumaWidth = detail::alignDimension(totalWidth, widthAlignment, 0, 1);
                    const int alignedChromaWidth = detail::alignDimension(totalWidth, widthAlignment, 1, 1);

                    channels[0].rowStride = alignedLumaWidth;
                    channels[0].pixelStride = 1;

                    channels[1].rowStride = alignedChromaWidth;
                    channels[1].pixelStride = 1;

                    channels[2].rowStride = alignedChromaWidth;
                    channels[2].pixelStride = 1;
                    break;
                }

                case ImageLayout::NV12: {
                    const int alignedWidth = detail::alignDimension(totalWidth, widthAlignment, 0, 1);

                    channels[0].rowStride = alignedWidth;
                    channels[0].pixelStride = 1;

                    channels[1].rowStride = alignedWidth;
                    channels[1].pixelStride = 2;

                    channels[2].rowStride = alignedWidth;
                    channels[2].pixelStride = 2;
                    break;
                }

                case ImageLayout::CUSTOM: {
                    const int maxSubsample = maxSubsampleValue();

                    for (int i = 0; i < numChannels; ++i) {
                        channels[i].rowStride = detail::alignDimension(
                                totalWidth, widthAlignment, channels[i].subsample, maxSubsample);
                        channels[i].pixelStride = 1;
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }
        }

        // Compute slice strides from the channel's XY dimensions; depth has no border.
        const int totalHeight = height + 2 * border;
        const int maxSubsample = maxSubsampleValue();

        for (int i = 0; i < numChannels; ++i) {
            if (channels[i].sliceStride > 0) {
                continue;
            }

            if (imageLayout == ImageLayout::NV12) {
                // NV12 chroma rows occupy half the aligned luma height.
                const int alignedHeight = detail::alignDimension(totalHeight, heightAlignment, 0, 1);
                channels[i].sliceStride = channels[i].rowStride * (alignedHeight >> channels[i].subsample);
            } else {
                channels[i].sliceStride = channels[i].rowStride *
                                          detail::alignDimension(
                                                  totalHeight, heightAlignment, channels[i].subsample, maxSubsample);
            }
        }

        // Compute channel offsets
        const bool computeOffsets = [&]() {
            for (int i = 0; i < numChannels; ++i) {
                if (channels[0].offset < 0) {
                    return true;
                }
            }
            return false; // we do not want to override user-provided offsets
        }();

        if (computeOffsets) {
            switch (imageLayout) {
                case ImageLayout::PLANAR: {
                    const int64_t numPixelPerChannel = channelBufferSize(0);

                    for (unsigned i = 0; i < channels.size(); ++i) {
                        channels[i].offset = i * numPixelPerChannel;
                    }
                    break;
                }

                case ImageLayout::INTERLEAVED: {
                    for (unsigned i = 0; i < channels.size(); ++i) {
                        channels[i].offset = i;
                    }
                    break;
                }

                case ImageLayout::YUV_420: {
                    const int64_t numPixelLumaChannel = channelBufferSize(0);
                    const int64_t numPixelChromaChannel = channelBufferSize(1);

                    channels[0].offset = 0;
                    channels[1].offset = numPixelLumaChannel;
                    channels[2].offset = numPixelLumaChannel + numPixelChromaChannel;
                    break;
                }

                case ImageLayout::NV12: {
                    const int64_t numPixelLumaChannel = channelBufferSize(0);

                    channels[0].offset = 0;
                    channels[1].offset = numPixelLumaChannel;
                    channels[2].offset = numPixelLumaChannel + 1;
                    break;
                }

                case ImageLayout::CUSTOM: {
                    int64_t offset = 0;
                    for (int i = 0; i < numChannels; ++i) {
                        channels[i].offset = offset;

                        offset += channelBufferSize(i);
                    }
                    break;
                }

                default:
                    throw std::invalid_argument("Invalid image layout "s + toString(imageLayout));
            }

            // Add offset for image border
            if (border > 0) {
                for (int i = 0; i < numChannels; ++i) {
                    const int x = border >> channels[i].subsample;
                    const int y = border >> channels[i].subsample;
                    const int64_t borderOffset = y * channels[i].rowStride + x * channels[i].pixelStride;
                    channels[i].offset += borderOffset;
                }
            }
        }
    }
};

class LayoutDescriptor::Builder final {
public:
    /// Starts from an existing layout, recomputing standard channel layouts on build.
    explicit Builder(const LayoutDescriptor& descriptor) : mDescriptor(descriptor) {
        if (mDescriptor.imageLayout != ImageLayout::CUSTOM) {
            invalidateChannels();
        }
    }

    /// Starts a layout with the given spatial dimensions; 2D images have one depth slice.
    Builder(int width, int height, int depth = 1) noexcept {
        mDescriptor.width = width;
        mDescriptor.height = height;
        mDescriptor.depth = depth;
    }

    Builder& imageLayout(ImageLayout imageLayout) noexcept {
        mDescriptor.imageLayout = imageLayout;
        return *this;
    }

    Builder& pixelType(PixelType pixelType) noexcept {
        mDescriptor.pixelType = pixelType;
        return *this;
    }

    Builder& pixelPrecision(int pixelPrecision) noexcept {
        mDescriptor.pixelPrecision = pixelPrecision;
        return *this;
    }

    Builder& width(int width) noexcept {
        mDescriptor.width = width;
        invalidateChannels();
        return *this;
    }

    /// Sets the slice height and recomputes channel layout on build.
    Builder& height(int height) noexcept {
        mDescriptor.height = height;
        invalidateChannels();
        return *this;
    }

    /// Sets the number of depth slices and recomputes channel layout on build.
    Builder& depth(int depth) noexcept {
        mDescriptor.depth = depth;
        invalidateChannels();
        return *this;
    }

    /// Sets the number of image channels.
    Builder& numChannels(int numChannels) noexcept {
        mDescriptor.numChannels = numChannels;
        return *this;
    }

    Builder& widthAlignment(int widthAlignment) noexcept {
        mDescriptor.widthAlignment = widthAlignment;
        return *this;
    }

    Builder& heightAlignment(int heightAlignment) noexcept {
        mDescriptor.heightAlignment = heightAlignment;
        return *this;
    }

    /// Sets depth padding and recomputes channel offsets, preserving custom strides.
    Builder& depthAlignment(int depthAlignment) noexcept {
        mDescriptor.depthAlignment = depthAlignment;

        for (auto& channel : mDescriptor.channels) {
            channel.offset = -1;
        }

        return *this;
    }

    Builder& sizeAlignment(int sizeAlignment) noexcept {
        mDescriptor.sizeAlignment = sizeAlignment;
        return *this;
    }

    Builder& border(int border) noexcept {
        mDescriptor.border = border;
        return *this;
    }

    /// Sets a custom channel's spatial subsampling factor as a power of two.
    Builder& channelSubsample(int index, int subsample) {
        mDescriptor.channels[index].subsample = subsample;
        return *this;
    }

    /// Sets a channel's offset from the start of the image buffer, in pixels.
    Builder& channelOffset(int index, int64_t offset) {
        mDescriptor.channels[index].offset = offset;
        return *this;
    }

    /// Sets strides in pixels; a zero slice stride is computed from the aligned slice height.
    Builder& channelStrides(int index, int64_t rowStride, int64_t pixelStride = 1, int64_t sliceStride = 0) {
        mDescriptor.channels[index].rowStride = rowStride;
        mDescriptor.channels[index].pixelStride = pixelStride;
        mDescriptor.channels[index].sliceStride = sliceStride;
        return *this;
    }

    /// Validates dimensions and computes any missing channel layout values.
    LayoutDescriptor build() {
        // Force YUV pixel type for YUV layouts
        if (model::isYuvLayout(mDescriptor.imageLayout)) {
            pixelType(PixelType::YUV);
        }

        // Force number of channels depending on pixel type
        const int numChannelsForPixelType = model::pixelNumChannels(mDescriptor.pixelType);
        if (numChannelsForPixelType > 0) {
            numChannels(numChannelsForPixelType);
        }

        // Force planar layout for grayscale and bayer pixels
        if (mDescriptor.pixelType == PixelType::GRAYSCALE || model::isBayerPixelType(mDescriptor.pixelType)) {
            imageLayout(ImageLayout::PLANAR);
        }

        // Validity checks
        if (mDescriptor.width <= 0 || mDescriptor.height <= 0 || mDescriptor.depth <= 0 ||
            mDescriptor.numChannels <= 0) {
            throw std::invalid_argument("Image dimension parameters (width=" + std::to_string(mDescriptor.width) +
                                        ", height=" + std::to_string(mDescriptor.height) +
                                        ", depth=" + std::to_string(mDescriptor.depth) +
                                        ", numChannels=" + std::to_string(mDescriptor.numChannels) +
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
        if (!math::isPowerOf2(mDescriptor.depthAlignment)) {
            throw std::invalid_argument("depthAlignment (" + std::to_string(mDescriptor.depthAlignment) +
                                        ") must be a power of 2.");
        }
        if (!math::isPowerOf2(mDescriptor.sizeAlignment)) {
            throw std::invalid_argument("sizeAlignment (" + std::to_string(mDescriptor.sizeAlignment) +
                                        ") must be a power of 2.");
        }
        if (mDescriptor.numChannels > image::detail::MAX_NUM_CHANNELS) {
            throw std::invalid_argument("Image number of channels (" + std::to_string(mDescriptor.numChannels) +
                                        ") exceeds limits (" + std::to_string(image::detail::MAX_NUM_CHANNELS) + ").");
        }

        // Compute channel offsets and strides
        mDescriptor.updateChannels();

        return mDescriptor;
    }

private:
    /// Clears derived offsets and strides so they are recomputed for new dimensions.
    void invalidateChannels() {
        for (unsigned i = 0; i < mDescriptor.channels.size(); ++i) {
            channelOffset(i, -1);
            channelStrides(i, 0);
        }
    }

    LayoutDescriptor mDescriptor;
};

} // namespace cxximg
