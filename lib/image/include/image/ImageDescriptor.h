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

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cxximg {

/// Structure describing generic plane layout.
/// @ingroup image
template <typename T>
struct PlaneDescriptor final {
    class Builder;

    int index = 0;           ///< Plane index in image
    int subsample = 0;       ///< Plane subsample comparing to image size, in power of two.
    T *buffer = nullptr;     ///< Pointer to buffer containing plane pixels.
    int64_t rowStride = 0;   ///< Distance between adjacent plane rows, in pixels.
    int64_t pixelStride = 1; ///< Distance between adjacent plane pixels, in pixels.

    PlaneDescriptor() = default;

    template <typename U>
    explicit PlaneDescriptor(const PlaneDescriptor<U> &descriptor)
        : index(descriptor.index),
          buffer(reinterpret_cast<T *>(descriptor.buffer)),
          rowStride(descriptor.rowStride),
          pixelStride(descriptor.pixelStride),
          subsample(descriptor.subsample) {}
};

template <typename T>
class PlaneDescriptor<T>::Builder final {
public:
    explicit Builder(T *buffer = nullptr) : mBuffer(buffer) {}

    Builder &offset(int64_t offset) {
        mOffset = offset;
        return *this;
    }

    Builder &rowStride(int rowStride) {
        mDescriptor.rowStride = rowStride;
        return *this;
    }

    Builder &pixelStride(int pixelStride) {
        mDescriptor.pixelStride = pixelStride;
        return *this;
    }

    Builder &subsample(int subsample) {
        mDescriptor.subsample = subsample;
        return *this;
    }

    PlaneDescriptor<T> build() const noexcept {
        PlaneDescriptor<T> descriptor(mDescriptor);
        if (mBuffer != nullptr) {
            descriptor.buffer = mBuffer + mOffset;
        }

        return descriptor;
    }

private:
    PlaneDescriptor<T> mDescriptor;
    T *mBuffer;
    int64_t mOffset = 0;
};

using PlaneDescriptor8i = PlaneDescriptor<int8_t>;
using PlaneDescriptor16i = PlaneDescriptor<int16_t>;
using PlaneDescriptor32i = PlaneDescriptor<int32_t>;

using PlaneDescriptor8u = PlaneDescriptor<uint8_t>;
using PlaneDescriptor16u = PlaneDescriptor<uint16_t>;
using PlaneDescriptor32u = PlaneDescriptor<uint32_t>;

using PlaneDescriptorf = PlaneDescriptor<float>;
using PlaneDescriptord = PlaneDescriptor<double>;

template <typename T>
using PlaneDescriptorArray = std::array<PlaneDescriptor<T>, image::detail::MAX_NUM_PLANES>;

/// Structure describing generic image layout.
/// @ingroup image
template <typename T>
struct ImageDescriptor final {
    LayoutDescriptor layout;        ///< Image layout descriptor;
    PlaneDescriptorArray<T> planes; ///< Image planes.

    template <typename U>
    explicit ImageDescriptor(const ImageDescriptor<U> &descriptor) : layout(descriptor.layout) {
        for (size_t i = 0; i < descriptor.planes.size(); ++i) {
            planes[i] = PlaneDescriptor<T>(descriptor.planes[i]);
        }
    }

    ImageDescriptor(const LayoutDescriptor &layout_, // NOLINT(google-explicit-constructor)
                    const PlaneDescriptorArray<T> &planes_ = {})
        : layout(layout_), planes(planes_) {
        using namespace std::string_literals;

        // Set plane indices
        for (size_t i = 0; i < planes.size(); ++i) {
            planes[i].index = i;
        }

        // Set plane subsample from layout
        switch (layout.imageLayout) {
            case ImageLayout::PLANAR:
            case ImageLayout::INTERLEAVED:
                for (size_t i = 0; i < planes.size(); ++i) {
                    planes[i].subsample = 0;
                }
                break;

            case ImageLayout::YUV_420:
            case ImageLayout::NV12:
                if (layout.numPlanes != 3) {
                    throw std::invalid_argument("YUV image number of planes must be 3.");
                }

                planes[0].subsample = 0;
                planes[1].subsample = 1;
                planes[2].subsample = 1;
                break;

            case ImageLayout::CUSTOM:
                // Nothing to do
                break;

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(layout.imageLayout));
        }
    }

    /// Compute the maximum value that can be represented by the image pixel precision.
    T saturationValue() const noexcept { return layout.saturationValue<T>(); }

    /// Compute required buffer size in order to store pixels.
    int64_t requiredBufferSize() const {
        if (layout.imageLayout != ImageLayout::CUSTOM) {
            return layout.requiredBufferSize();
        }

        const int maxSubsample = computeMaxSubsample();

        // Compute the amount of contiguous memory we need, taking into account planes subsample
        int64_t bufferSize = 0;
        for (int i = 0; i < layout.numPlanes; ++i) {
            const int alignedWidth = detail::alignDimension(
                    layout.width, layout.widthAlignment, planes[i].subsample, maxSubsample);
            const int alignedHeight = detail::alignDimension(layout.height, 1, planes[i].subsample, maxSubsample);

            bufferSize += static_cast<int64_t>(alignedWidth) * alignedHeight;
        }

        return bufferSize;
    }

    /// Compute plane strides from layout descriptor.
    void computeStrides() {
        using namespace std::string_literals;

        switch (layout.imageLayout) {
            case ImageLayout::PLANAR: {
                const int alignedWidth = detail::alignDimension(layout.width, layout.widthAlignment);
                for (size_t i = 0; i < planes.size(); ++i) {
                    planes[i].rowStride = alignedWidth;
                    planes[i].pixelStride = 1;
                }
                break;
            }

            case ImageLayout::INTERLEAVED: {
                const int alignedWidth = detail::alignDimension(layout.numPlanes * layout.width, layout.widthAlignment);
                for (size_t i = 0; i < planes.size(); ++i) {
                    planes[i].rowStride = alignedWidth;
                    planes[i].pixelStride = layout.numPlanes;
                }
                break;
            }

            case ImageLayout::YUV_420: {
                if (layout.numPlanes != 3) {
                    throw std::invalid_argument("YUV image number of planes must be 3.");
                }

                const int alignedLumaWidth = detail::alignDimension(layout.width, layout.widthAlignment, 0, 1);
                const int alignedChromaWidth = detail::alignDimension(layout.width, layout.widthAlignment, 1, 1);

                planes[0].rowStride = alignedLumaWidth;
                planes[0].pixelStride = 1;

                planes[1].rowStride = alignedChromaWidth;
                planes[1].pixelStride = 1;

                planes[2].rowStride = alignedChromaWidth;
                planes[2].pixelStride = 1;
                break;
            }

            case ImageLayout::NV12: {
                if (layout.numPlanes != 3) {
                    throw std::invalid_argument("NV12 image number of planes must be 3.");
                }

                const int alignedWidth = detail::alignDimension(layout.width, layout.widthAlignment, 0, 1);

                planes[0].rowStride = alignedWidth;
                planes[0].pixelStride = 1;

                planes[1].rowStride = alignedWidth;
                planes[1].pixelStride = 2;

                planes[2].rowStride = alignedWidth;
                planes[2].pixelStride = 2;

                break;
            }

            case ImageLayout::CUSTOM: {
                const int maxSubsample = computeMaxSubsample();

                for (int i = 0; i < layout.numPlanes; ++i) {
                    planes[i].rowStride = detail::alignDimension(
                            layout.width, layout.widthAlignment, planes[i].subsample, maxSubsample);
                    planes[i].pixelStride = 1;
                }
                break;
            }

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(layout.imageLayout));
        }
    }

    /// Map this descriptor to the given buffer.
    ImageDescriptor<T> &map(T *buffer) {
        using namespace std::string_literals;

        if (buffer == nullptr) {
            for (size_t i = 0; i < planes.size(); ++i) {
                planes[i].buffer = nullptr;
            }
            return *this;
        }

        // Ensure we have valid strides
        if (!planes[0].rowStride) {
            computeStrides();
        }

        switch (layout.imageLayout) {
            case ImageLayout::PLANAR: {
                const int64_t numPixelsPerPlane = planes[0].rowStride * layout.height;

                for (size_t i = 0; i < planes.size(); ++i) {
                    planes[i].buffer = buffer + i * numPixelsPerPlane;
                }
                break;
            }

            case ImageLayout::INTERLEAVED:
                for (size_t i = 0; i < planes.size(); ++i) {
                    planes[i].buffer = buffer + i;
                }
                break;

            case ImageLayout::YUV_420: {
                const int64_t numPixelLumaPlane = planes[0].rowStride * detail::alignDimension(layout.height, 1, 0, 1);
                const int64_t numPixelChromaPlane = planes[1].rowStride *
                                                    detail::alignDimension(layout.height, 1, 1, 1);

                planes[0].buffer = buffer;
                planes[1].buffer = buffer + numPixelLumaPlane;
                planes[2].buffer = buffer + numPixelLumaPlane + numPixelChromaPlane;
                break;
            }

            case ImageLayout::NV12: {
                const int64_t numPixelLumaPlane = planes[0].rowStride * detail::alignDimension(layout.height, 1, 0, 1);

                planes[0].buffer = buffer;
                planes[1].buffer = buffer + numPixelLumaPlane;
                planes[2].buffer = buffer + numPixelLumaPlane + 1;
                break;
            }

            case ImageLayout::CUSTOM: {
                const int maxSubsample = computeMaxSubsample();

                int64_t offset = 0;
                for (int i = 0; i < layout.numPlanes; ++i) {
                    planes[i].buffer = buffer + offset;

                    const int alignedHeight = detail::alignDimension(
                            layout.height, 1, planes[i].subsample, maxSubsample);

                    offset += planes[i].rowStride * alignedHeight;
                }
                break;
            }

            default:
                throw std::invalid_argument("Invalid image layout "s + toString(layout.imageLayout));
        }

        return *this;
    }

private:
    int computeMaxSubsample() const {
        const auto &plane = std::max_element(planes.begin(),
                                             planes.begin() + layout.numPlanes,
                                             [](const auto &r, const auto &l) { return r.subsample < l.subsample; });

        return plane->subsample;
    }
};

using ImageDescriptor8i = ImageDescriptor<int8_t>;
using ImageDescriptor16i = ImageDescriptor<int16_t>;
using ImageDescriptor32i = ImageDescriptor<int32_t>;

using ImageDescriptor8u = ImageDescriptor<uint8_t>;
using ImageDescriptor16u = ImageDescriptor<uint16_t>;
using ImageDescriptor32u = ImageDescriptor<uint32_t>;

using ImageDescriptorf = ImageDescriptor<float>;
using ImageDescriptord = ImageDescriptor<double>;

namespace image {

/// Computes a four planes (R, Gr, Gb, B) descriptor from a one plane bayer layout.
template <typename T>
ImageDescriptor<T> computeBayerPlanarDescriptor(const ImageDescriptor<T> &rawDescriptor) {
    const LayoutDescriptor &rawLayout = rawDescriptor.layout;
    const int alignedWidth = cxximg::detail::alignDimension(rawLayout.width, rawLayout.widthAlignment);
    T *buffer = rawDescriptor.planes[0].buffer;

    return {LayoutDescriptor::Builder(rawLayout.width / 2, rawLayout.height / 2)
                    .numPlanes(4)
                    .imageLayout(ImageLayout::CUSTOM)
                    .pixelPrecision(rawLayout.pixelPrecision)
                    .build(),
            {{typename PlaneDescriptor<T>::Builder(buffer)
                      .offset(bayerYOffset(rawLayout.pixelType, Bayer::R) * alignedWidth +
                              bayerXOffset(rawLayout.pixelType, Bayer::R))
                      .rowStride(2 * alignedWidth)
                      .pixelStride(2)
                      .build(),
              typename PlaneDescriptor<T>::Builder(buffer)
                      .offset(bayerYOffset(rawLayout.pixelType, Bayer::GR) * alignedWidth +
                              bayerXOffset(rawLayout.pixelType, Bayer::GR))
                      .rowStride(2 * alignedWidth)
                      .pixelStride(2)
                      .build(),
              typename PlaneDescriptor<T>::Builder(buffer)
                      .offset(bayerYOffset(rawLayout.pixelType, Bayer::GB) * alignedWidth +
                              bayerXOffset(rawLayout.pixelType, Bayer::GB))
                      .rowStride(2 * alignedWidth)
                      .pixelStride(2)
                      .build(),
              typename PlaneDescriptor<T>::Builder(buffer)
                      .offset(bayerYOffset(rawLayout.pixelType, Bayer::B) * alignedWidth +
                              bayerXOffset(rawLayout.pixelType, Bayer::B))
                      .rowStride(2 * alignedWidth)
                      .pixelStride(2)
                      .build()}}};
}

/// Computes the subset of the input descriptor given the ROI coordinates.
template <typename T>
ImageDescriptor<T> computeRoiDescriptor(const ImageDescriptor<T> &descriptor, const Roi &roi) {
    PlaneDescriptorArray<T> planes(descriptor.planes);
    for (int i = 0; i < descriptor.layout.numPlanes; ++i) {
        const int x = roi.x >> planes[i].subsample;
        const int y = roi.y >> planes[i].subsample;
        const int64_t offset = y * planes[i].rowStride + x * planes[i].pixelStride;
        planes[i].buffer += offset;
    }

    return {LayoutDescriptor::Builder(descriptor.layout).width(roi.width).height(roi.height).build(), planes};
}

} // namespace image

} // namespace cxximg
