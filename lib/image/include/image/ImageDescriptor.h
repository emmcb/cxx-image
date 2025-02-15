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

#include "image/LayoutDescriptor.h"
#include "image/Roi.h"
#include "math/half.h"

#include <array>
#include <cstdint>

#ifdef CXXIMG_HAVE_HALIDE
#include <memory>
#include <type_traits>

#include <HalideRuntime.h>
#endif

namespace cxximg {

#ifdef CXXIMG_HAVE_HALIDE
struct HalideDescriptor final {
    halide_buffer_t buffer;
    std::array<halide_dimension_t, 3> dim;
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

    void syncDims(const LayoutDescriptor &layout) {
        if (layout.numPlanes > 1) {
            buffer.dimensions = 3;
        } else if (layout.height > 1) {
            buffer.dimensions = 2;
        } else {
            buffer.dimensions = 1;
        }

        dim[0].min = -layout.border;
        dim[0].extent = layout.width + 2 * layout.border;
        dim[0].stride = layout.planes[0].pixelStride;

        dim[1].min = -layout.border;
        dim[1].extent = layout.height + 2 * layout.border;
        dim[1].stride = layout.planes[0].rowStride;

        dim[2].min = 0;
        dim[2].extent = layout.numPlanes;
        dim[2].stride = layout.planes[1].offset - layout.planes[0].offset;
    }
};
#endif

/// Structure describing generic image layout.
/// @ingroup image
template <typename T>
struct ImageDescriptor final {
    LayoutDescriptor layout; ///< Image layout descriptor;
    T *buffer;               ///< Image buffer.

#ifdef CXXIMG_HAVE_HALIDE
    std::shared_ptr<HalideDescriptor> halide = std::make_shared<HalideDescriptor>(); ///< Halide descriptor.
#endif

    ImageDescriptor(const LayoutDescriptor &layout_, T *buffer_) : layout(layout_), buffer(buffer_) {
#ifdef CXXIMG_HAVE_HALIDE
        resetHalideDescriptor();
#endif
    }

    /// Map this descriptor to the given buffer.
    ImageDescriptor<T> &map(T *buffer_) {
        buffer = buffer_;

#ifdef CXXIMG_HAVE_HALIDE
        resetHalideDescriptor();
#endif

        return *this;
    }

    /// Compute the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return layout.saturationValue<T>(); }

private:
#ifdef CXXIMG_HAVE_HALIDE
    template <typename U = T, std::enable_if_t<math::is_arithmetic_v<U>, bool> = true>
    void resetHalideDescriptor() {
        halide->buffer.type = halide_type_of<T>();
        halide->buffer.host = reinterpret_cast<uint8_t *>(buffer);
        halide->buffer.device = 0;
        halide->buffer.device_interface = nullptr;
        halide->buffer.flags = 0;
        halide->syncDims(layout);
    }

    template <typename U = T, std::enable_if_t<!math::is_arithmetic_v<U>, bool> = true>
    void resetHalideDescriptor() {}
#endif
};

namespace image {

/// Computes a four planes (R, Gr, Gb, B) descriptor from a one plane bayer layout.
template <typename T>
ImageDescriptor<T> computeBayerPlanarDescriptor(const ImageDescriptor<T> &bayerDescriptor) {
    const LayoutDescriptor &bayerLayout = bayerDescriptor.layout;
    const int64_t rowStride = bayerLayout.planes[0].rowStride;

    const auto computeOffset = [&](Bayer bayer) {
        return bayerYOffset(bayerLayout.pixelType, bayer) * rowStride + bayerXOffset(bayerLayout.pixelType, bayer);
    };

    return ImageDescriptor<T>(LayoutDescriptor::Builder(bayerLayout.width / 2, bayerLayout.height / 2)
                                      .numPlanes(4)
                                      .imageLayout(ImageLayout::CUSTOM)
                                      .pixelPrecision(bayerLayout.pixelPrecision)
                                      .planeOffset(0, computeOffset(Bayer::R))
                                      .planeOffset(1, computeOffset(Bayer::GR))
                                      .planeOffset(2, computeOffset(Bayer::GB))
                                      .planeOffset(3, computeOffset(Bayer::B))
                                      .planeStrides(0, 2 * rowStride, 2)
                                      .planeStrides(1, 2 * rowStride, 2)
                                      .planeStrides(2, 2 * rowStride, 2)
                                      .planeStrides(3, 2 * rowStride, 2)
                                      .build(),
                              bayerDescriptor.buffer);
}

/// Computes the subset of the input descriptor given the ROI coordinates.
template <typename T>
ImageDescriptor<T> computeRoiDescriptor(const ImageDescriptor<T> &descriptor, const Roi &roi) {
    LayoutDescriptor::Builder builder(descriptor.layout);
    builder.width(roi.width).height(roi.height).border(0);

    for (int i = 0; i < descriptor.layout.numPlanes; ++i) {
        const auto &plane = descriptor.layout.planes[i];

        const int x = roi.x >> plane.subsample;
        const int y = roi.y >> plane.subsample;
        const int64_t roiOffset = y * plane.rowStride + x * plane.pixelStride;

        builder.planeSubsample(i, plane.subsample);
        builder.planeOffset(i, plane.offset + roiOffset);
        builder.planeStrides(i, plane.rowStride, plane.pixelStride);
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

using ImageDescriptorh = ImageDescriptor<half_t>;
using ImageDescriptorf = ImageDescriptor<float>;
using ImageDescriptord = ImageDescriptor<double>;

} // namespace cxximg
