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

#include <optional>
#include <string>

namespace cxximg {

/// Image layout description.
/// @ingroup model
enum class ImageLayout {
    /// Custom
    CUSTOM,

    /// Contiguous planes of same size
    PLANAR,

    /// Interleaved planes of same size
    INTERLEAVED,

    /// YUV 420: planar YUV with full size Y and subsampled UV
    YUV_420,

    /// Semi planar YUV: full size planar Y and subsampled interleaved UV
    NV12
};

inline const char *toString(ImageLayout imageLayout) {
    switch (imageLayout) {
        case ImageLayout::CUSTOM:
            return "custom";
        case ImageLayout::PLANAR:
            return "planar";
        case ImageLayout::INTERLEAVED:
            return "interleaved";
        case ImageLayout::YUV_420:
            return "yuv_420";
        case ImageLayout::NV12:
            return "nv12";
    }
    return "undefined";
}

inline std::optional<ImageLayout> parseImageLayout(const std::string &imageLayout) {
    if (imageLayout == "custom") {
        return ImageLayout::CUSTOM;
    }
    if (imageLayout == "planar") {
        return ImageLayout::PLANAR;
    }
    if (imageLayout == "interleaved") {
        return ImageLayout::INTERLEAVED;
    }
    if (imageLayout == "yuv_420") {
        return ImageLayout::YUV_420;
    }
    if (imageLayout == "nv12") {
        return ImageLayout::NV12;
    }
    return std::nullopt;
}

namespace model {

/// Checks whether the given image layout is a YUV-type layout (for example YUV420 or NV12).
inline bool isYuvLayout(ImageLayout layout) {
    return layout == ImageLayout::YUV_420 || layout == ImageLayout::NV12;
}

} // namespace model

} // namespace cxximg
