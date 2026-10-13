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

#include "cxximg/image/LayoutDescriptor.h"

#include "cxximg/math/Rect.h"
#include "cxximg/math/half.h"

#include <array>
#include <cstdint>
#include <stdexcept>

#ifdef CXXIMG_HAVE_HALIDE
#include <memory>
#include <type_traits>

#include <HalideRuntime.h>
#endif

namespace cxximg {

#ifdef CXXIMG_HAVE_HALIDE
struct HalideDescriptor final {
    halide_buffer_t buffer;
    std::array<halide_dimension_t, 4> dim;
    bool isCrop = false;

    HalideDescriptor() { buffer.dim = dim.data(); }

    ~HalideDescriptor() {
        if (buffer.device != 0) {
            if (isCrop) {
                halide_device_release_crop(nullptr, &buffer);
            } else {
                halide_device_free(nullptr, &buffer);
            }
        }
    }

    /// Synchronizes spatial dimensions and channels, using depth as dimension 2 for single-channel volumes.
    void syncDims(const LayoutDescriptor& layout) {
        if (layout.depth > 1 && layout.numChannels > 1) {
            buffer.dimensions = 4;
        } else if (layout.depth > 1 || layout.numChannels > 1) {
            buffer.dimensions = 3;
        } else if (layout.height > 1) {
            buffer.dimensions = 2;
        } else {
            buffer.dimensions = 1;
        }

        dim[0].min = -layout.border;
        dim[0].extent = layout.width + 2 * layout.border;
        dim[0].stride = layout.channels[0].pixelStride;

        dim[1].min = -layout.border;
        dim[1].extent = layout.height + 2 * layout.border;
        dim[1].stride = layout.channels[0].rowStride;

        dim[2].min = 0;
        if (layout.numChannels > 1) {
            dim[2].extent = layout.numChannels;
            dim[2].stride = layout.channels[1].offset - layout.channels[0].offset;
        } else {
            dim[2].extent = layout.depth;
            dim[2].stride = layout.channels[0].sliceStride;
        }

        dim[3].min = 0;
        dim[3].extent = layout.depth;
        dim[3].stride = layout.channels[0].sliceStride;
    }
};
#endif

/// Structure describing generic image layout.
/// @ingroup image
template <typename T>
struct ImageDescriptor final {
    LayoutDescriptor layout; ///< Image layout descriptor;
    T* buffer;               ///< Image buffer.

#ifdef CXXIMG_HAVE_HALIDE
    std::shared_ptr<HalideDescriptor> halide = std::make_shared<HalideDescriptor>(); ///< Halide descriptor.
#endif

    ImageDescriptor(const LayoutDescriptor& layout_, T* buffer_) : layout(layout_), buffer(buffer_) {
#ifdef CXXIMG_HAVE_HALIDE
        resetHalideDescriptor();
#endif
    }

    /// Map this descriptor to the given buffer.
    ImageDescriptor<T>& map(T* buffer_) {
        buffer = buffer_;

#ifdef CXXIMG_HAVE_HALIDE
        halide->buffer.host = reinterpret_cast<uint8_t*>(buffer);
#endif

        return *this;
    }

    /// Compute the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return layout.saturationValue<T>(); }

private:
#ifdef CXXIMG_HAVE_HALIDE
    void resetHalideDescriptor() {
        if constexpr (math::is_arithmetic_v<T>) {
            halide->buffer.type = halide_type_of<T>();
        }

        halide->buffer.host = reinterpret_cast<uint8_t*>(buffer);
        halide->buffer.device = 0;
        halide->buffer.device_interface = nullptr;
        halide->buffer.flags = 0;
        halide->syncDims(layout);
    }
#endif
};

namespace image {

/// Computes four channels (R, Gr, Gb, B) from a 2D, single-channel Bayer layout.
template <typename T>
ImageDescriptor<T> computeBayerPlanarDescriptor(const ImageDescriptor<T>& bayerDescriptor) {
    const LayoutDescriptor& bayerLayout = bayerDescriptor.layout;
    const int64_t rowStride = bayerLayout.channels[0].rowStride;

    const auto computeOffset = [&](Bayer bayer) {
        return model::bayerOffsetY(bayerLayout.pixelType, bayer) * rowStride +
               model::bayerOffsetX(bayerLayout.pixelType, bayer);
    };

    return ImageDescriptor<T>(LayoutDescriptor::Builder(bayerLayout.width / 2, bayerLayout.height / 2)
                                      .numChannels(4)
                                      .imageLayout(ImageLayout::CUSTOM)
                                      .pixelPrecision(bayerLayout.pixelPrecision)
                                      .channelOffset(0, computeOffset(Bayer::R))
                                      .channelOffset(1, computeOffset(Bayer::GR))
                                      .channelOffset(2, computeOffset(Bayer::GB))
                                      .channelOffset(3, computeOffset(Bayer::B))
                                      .channelStrides(0, 2 * rowStride, 2, bayerLayout.channels[0].sliceStride)
                                      .channelStrides(1, 2 * rowStride, 2, bayerLayout.channels[0].sliceStride)
                                      .channelStrides(2, 2 * rowStride, 2, bayerLayout.channels[0].sliceStride)
                                      .channelStrides(3, 2 * rowStride, 2, bayerLayout.channels[0].sliceStride)
                                      .build(),
                              bayerDescriptor.buffer);
}

/// Computes the subset of the input descriptor given the ROI coordinates.
template <typename T>
ImageDescriptor<T> computeRoiDescriptor(const ImageDescriptor<T>& descriptor, const Rect& roi) {
    LayoutDescriptor::Builder builder(descriptor.layout);
    builder.width(roi.width).height(roi.height).border(0);

    for (int i = 0; i < descriptor.layout.numChannels; ++i) {
        const auto& channel = descriptor.layout.channels[i];

        const int x = roi.x >> channel.subsample;
        const int y = roi.y >> channel.subsample;
        const int64_t roiOffset = y * channel.rowStride + x * channel.pixelStride;

        builder.channelSubsample(i, channel.subsample);
        builder.channelOffset(i, channel.offset + roiOffset);
        builder.channelStrides(i, channel.rowStride, channel.pixelStride, channel.sliceStride);
    }

    ImageDescriptor<T> crop(builder.build(), descriptor.buffer);

#ifdef CXXIMG_HAVE_HALIDE
    if (descriptor.halide->buffer.device != 0) {
        crop.halide->dim[0].min = roi.x;
        crop.halide->dim[1].min = roi.y;

        if (halide_device_crop(nullptr, &descriptor.halide->buffer, &crop.halide->buffer) ==
            halide_error_code_success) {
            crop.halide->isCrop = true;
        }

        crop.halide->dim[0].min = 0;
        crop.halide->dim[1].min = 0;
    }
#endif

    return crop;
}

} // namespace image

using ImageDescriptor8i = ImageDescriptor<int8_t>;
using ImageDescriptor16i = ImageDescriptor<int16_t>;
using ImageDescriptor32i = ImageDescriptor<int32_t>;

using ImageDescriptor8u = ImageDescriptor<uint8_t>;
using ImageDescriptor16u = ImageDescriptor<uint16_t>;
using ImageDescriptor32u = ImageDescriptor<uint32_t>;

using ImageDescriptorh = ImageDescriptor<half>;
using ImageDescriptorf = ImageDescriptor<float>;
using ImageDescriptord = ImageDescriptor<double>;

} // namespace cxximg
