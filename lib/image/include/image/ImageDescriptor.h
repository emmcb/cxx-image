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
#include "image/detail/Alignment.h"

#include <array>
#include <cstdint>

#ifdef HAVE_HALIDE
#include <memory>
#include <type_traits>

#include <HalideRuntime.h>
#endif

namespace cxximg {

#ifdef HAVE_HALIDE
struct HalideDescriptor final {
    halide_buffer_t buffer;
    std::array<halide_dimension_t, 3> dim;

    HalideDescriptor() { buffer.dim = dim.data(); }

    ~HalideDescriptor() {
        if (buffer.device != 0) {
            halide_device_free(nullptr, &buffer);
        }
    }
};
#endif

template <typename T>
using BufferArray = std::array<T *, image::detail::MAX_NUM_PLANES>;

/// Structure describing generic image layout.
/// @ingroup image
template <typename T>
struct ImageDescriptor final {
    LayoutDescriptor layout; ///< Image layout descriptor;
    BufferArray<T> buffers;  ///< Image plane buffers.

#ifdef HAVE_HALIDE
    std::shared_ptr<HalideDescriptor> halide = std::make_shared<HalideDescriptor>(); ///< Halide descriptor.
#endif

    ImageDescriptor(const LayoutDescriptor &layout_, // NOLINT(google-explicit-constructor)
                    const BufferArray<T> &buffers_ = {})
        : layout(layout_), buffers(buffers_) {
#ifdef HAVE_HALIDE
        if (!buffers.empty()) {
            updateHalideDescriptor();
        }
#endif
    }

    /// Compute the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return layout.saturationValue<T>(); }

    /// Map this descriptor to the given buffer.
    ImageDescriptor<T> &map(T *buffer) {
        using namespace std::string_literals;

        if (buffer == nullptr) {
            for (auto &buffer : buffers) {
                buffer = nullptr;
            }
            return *this;
        }

        const int totalHeight = layout.height + 2 * layout.margin;

        switch (layout.imageLayout) {
            case ImageLayout::PLANAR: {
                const int64_t numPixelsPerPlane = layout.planes[0].rowStride * totalHeight;

                for (unsigned i = 0; i < layout.planes.size(); ++i) {
                    buffers[i] = buffer + i * numPixelsPerPlane;
                }
                break;
            }

            case ImageLayout::INTERLEAVED:
                for (unsigned i = 0; i < layout.planes.size(); ++i) {
                    buffers[i] = buffer + i;
                }
                break;

            case ImageLayout::YUV_420: {
                const int64_t numPixelLumaPlane = layout.planes[0].rowStride *
                                                  detail::alignDimension(totalHeight, 1, 0, 1);
                const int64_t numPixelChromaPlane = layout.planes[1].rowStride *
                                                    detail::alignDimension(totalHeight, 1, 1, 1);

                buffers[0] = buffer;
                buffers[1] = buffer + numPixelLumaPlane;
                buffers[2] = buffer + numPixelLumaPlane + numPixelChromaPlane;
                break;
            }

            case ImageLayout::NV12: {
                const int64_t numPixelLumaPlane = layout.planes[0].rowStride *
                                                  detail::alignDimension(totalHeight, 1, 0, 1);

                buffers[0] = buffer;
                buffers[1] = buffer + numPixelLumaPlane;
                buffers[2] = buffer + numPixelLumaPlane + 1;
                break;
            }

            case ImageLayout::CUSTOM: {
                const int maxSubsample = layout.maxSubsampleValue();

                int64_t offset = 0;
                for (int i = 0; i < layout.numPlanes; ++i) {
                    buffers[i] = buffer + offset;

                    const int alignedHeight = detail::alignDimension(
                            totalHeight, 1, layout.planes[i].subsample, maxSubsample);

                    offset += layout.planes[i].rowStride * alignedHeight;
                }
                break;
            }

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(layout.imageLayout));
        }

        if (layout.margin > 0) {
            for (int i = 0; i < layout.numPlanes; ++i) {
                const int x = layout.margin >> layout.planes[i].subsample;
                const int y = layout.margin >> layout.planes[i].subsample;
                const int64_t offset = y * layout.planes[i].rowStride + x * layout.planes[i].pixelStride;
                buffers[i] += offset;
            }
        }

#ifdef HAVE_HALIDE
        updateHalideDescriptor();
#endif

        return *this;
    }

private:
#ifdef HAVE_HALIDE
    template <typename U = T, std::enable_if_t<std::is_arithmetic_v<U>, bool> = true>
    void updateHalideDescriptor() {
        halide->dim[0].min = -layout.margin;
        halide->dim[0].extent = layout.width + 2 * layout.margin;
        halide->dim[0].stride = layout.planes[0].pixelStride;

        halide->dim[1].min = -layout.margin;
        halide->dim[1].extent = layout.height + 2 * layout.margin;
        halide->dim[1].stride = layout.planes[0].rowStride;

        halide->dim[2].min = 0;
        halide->dim[2].extent = layout.numPlanes;
        halide->dim[2].stride = buffers[1] - buffers[0];

        halide->buffer.dimensions = (layout.numPlanes > 1) ? 3 : 2;
        halide->buffer.host = reinterpret_cast<uint8_t *>(
                buffers[0] - layout.margin * (layout.planes[0].rowStride + layout.planes[0].pixelStride));
        halide->buffer.type = halide_type_of<T>();

        halide->buffer.device = 0;
        halide->buffer.device_interface = nullptr;
        halide->buffer.flags = 0;
    }

    template <typename U = T, std::enable_if_t<!std::is_arithmetic_v<U>, bool> = true>
    void updateHalideDescriptor() {}
#endif
};

namespace image {

/// Computes a four planes (R, Gr, Gb, B) descriptor from a one plane bayer layout.
template <typename T>
ImageDescriptor<T> computeBayerPlanarDescriptor(const ImageDescriptor<T> &bayerDescriptor) {
    const LayoutDescriptor &bayerLayout = bayerDescriptor.layout;
    const int64_t rowStride = bayerLayout.planes[0].rowStride;
    T *buffer = bayerDescriptor.buffers[0];

    const int rOffset = bayerYOffset(bayerLayout.pixelType, Bayer::R) * rowStride +
                        bayerXOffset(bayerLayout.pixelType, Bayer::R);
    const int grOffset = bayerYOffset(bayerLayout.pixelType, Bayer::GR) * rowStride +
                         bayerXOffset(bayerLayout.pixelType, Bayer::GR);
    const int gbOffset = bayerYOffset(bayerLayout.pixelType, Bayer::GB) * rowStride +
                         bayerXOffset(bayerLayout.pixelType, Bayer::GB);
    const int bOffset = bayerYOffset(bayerLayout.pixelType, Bayer::B) * rowStride +
                        bayerXOffset(bayerLayout.pixelType, Bayer::B);

    return ImageDescriptor<T>(LayoutDescriptor::Builder(bayerLayout.width / 2, bayerLayout.height / 2)
                                      .numPlanes(4)
                                      .imageLayout(ImageLayout::CUSTOM)
                                      .pixelPrecision(bayerLayout.pixelPrecision)
                                      .planeStrides(0, 2 * rowStride, 2)
                                      .planeStrides(1, 2 * rowStride, 2)
                                      .planeStrides(2, 2 * rowStride, 2)
                                      .planeStrides(3, 2 * rowStride, 2)
                                      .build(),
                              {buffer + rOffset, buffer + grOffset, buffer + gbOffset, buffer + bOffset});
}

/// Computes the subset of the input descriptor given the ROI coordinates.
template <typename T>
ImageDescriptor<T> computeRoiDescriptor(const ImageDescriptor<T> &descriptor, const Roi &roi) {
    BufferArray<T> buffers(descriptor.buffers);
    for (int i = 0; i < descriptor.layout.numPlanes; ++i) {
        const int x = roi.x >> descriptor.layout.planes[i].subsample;
        const int y = roi.y >> descriptor.layout.planes[i].subsample;
        const int64_t offset = y * descriptor.layout.planes[i].rowStride + x * descriptor.layout.planes[i].pixelStride;
        buffers[i] += offset;
    }

    LayoutDescriptor::Builder builder(descriptor.layout);
    builder.width(roi.width).height(roi.height);

    for (int i = 0; i < descriptor.layout.numPlanes; ++i) {
        builder.planeSubsample(i, descriptor.layout.planes[i].subsample);
        builder.planeStrides(i, descriptor.layout.planes[i].rowStride, descriptor.layout.planes[i].pixelStride);
    }

    return ImageDescriptor<T>(builder.build(), buffers);
}

} // namespace image

using ImageDescriptor8i = ImageDescriptor<int8_t>;
using ImageDescriptor16i = ImageDescriptor<int16_t>;
using ImageDescriptor32i = ImageDescriptor<int32_t>;

using ImageDescriptor8u = ImageDescriptor<uint8_t>;
using ImageDescriptor16u = ImageDescriptor<uint16_t>;
using ImageDescriptor32u = ImageDescriptor<uint32_t>;

using ImageDescriptorf = ImageDescriptor<float>;
using ImageDescriptord = ImageDescriptor<double>;

} // namespace cxximg
