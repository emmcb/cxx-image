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

#include <optional>
#include <stdexcept>
#include <string>

namespace cxximg {

/// Pixel layout description.
/// @ingroup model
enum class PixelType {
    /// Custom
    CUSTOM,

    /// Grayscale
    GRAYSCALE,

    /// Grayscale alongwith an alpha channel
    GRAY_ALPHA,

    /// RGB
    RGB,

    /// RGBA
    RGBA,

    /// YUV
    YUV,

    /// R G
    /// G B
    BAYER_RGGB,

    /// B G
    /// G R
    BAYER_BGGR,

    /// G R
    /// B G
    BAYER_GRBG,

    /// G B
    /// R G
    BAYER_GBRG,

    /// R R G G
    /// R R G G
    /// G G B B
    /// G G B B
    QUADBAYER_RGGB,

    /// B B G G
    /// B B G G
    /// G G R R
    /// G G R R
    QUADBAYER_BGGR,

    /// G G R R
    /// G G R R
    /// B B G G
    /// B B G G
    QUADBAYER_GRBG,

    /// G G B B
    /// G G B B
    /// R R G G
    /// R R G G
    QUADBAYER_GBRG
};

/// Bayer components.
/// @ingroup image
enum class Bayer { R, GR, GB, B };

inline const char *toString(PixelType pixelType) {
    switch (pixelType) {
        case PixelType::CUSTOM:
            return "custom";
        case PixelType::GRAYSCALE:
            return "grayscale";
        case PixelType::GRAY_ALPHA:
            return "gray_alpha";
        case PixelType::RGB:
            return "rgb";
        case PixelType::RGBA:
            return "rgba";
        case PixelType::YUV:
            return "yuv";
        case PixelType::BAYER_RGGB:
            return "bayer_rggb";
        case PixelType::BAYER_BGGR:
            return "bayer_bggr";
        case PixelType::BAYER_GRBG:
            return "bayer_grbg";
        case PixelType::BAYER_GBRG:
            return "bayer_gbrg";
        case PixelType::QUADBAYER_RGGB:
            return "quadbayer_rggb";
        case PixelType::QUADBAYER_BGGR:
            return "quadbayer_bggr";
        case PixelType::QUADBAYER_GRBG:
            return "quadbayer_grbg";
        case PixelType::QUADBAYER_GBRG:
            return "quadbayer_gbrg";
    }
    return "undefined";
}

inline std::optional<PixelType> parsePixelType(const std::string &pixelType) {
    if (pixelType == "custom") {
        return PixelType::CUSTOM;
    }
    if (pixelType == "grayscale") {
        return PixelType::GRAYSCALE;
    }
    if (pixelType == "gray_alpha") {
        return PixelType::GRAY_ALPHA;
    }
    if (pixelType == "rgb") {
        return PixelType::RGB;
    }
    if (pixelType == "rgba") {
        return PixelType::RGBA;
    }
    if (pixelType == "yuv") {
        return PixelType::YUV;
    }
    if (pixelType == "bayer_rggb") {
        return PixelType::BAYER_RGGB;
    }
    if (pixelType == "bayer_bggr") {
        return PixelType::BAYER_BGGR;
    }
    if (pixelType == "bayer_grbg") {
        return PixelType::BAYER_GRBG;
    }
    if (pixelType == "bayer_gbrg") {
        return PixelType::BAYER_GBRG;
    }
    if (pixelType == "quadbayer_rggb") {
        return PixelType::QUADBAYER_RGGB;
    }
    if (pixelType == "quadbayer_bggr") {
        return PixelType::QUADBAYER_BGGR;
    }
    if (pixelType == "quadbayer_grbg") {
        return PixelType::QUADBAYER_GRBG;
    }
    if (pixelType == "quadbayer_gbrg") {
        return PixelType::QUADBAYER_GBRG;
    }
    return std::nullopt;
}

namespace model {

/// Returns the number of image planes required by the given pixel type.
inline int pixelNumPlanes(PixelType pixelType) {
    switch (pixelType) {
        case PixelType::GRAYSCALE:
        case PixelType::BAYER_RGGB:
        case PixelType::BAYER_BGGR:
        case PixelType::BAYER_GRBG:
        case PixelType::BAYER_GBRG:
        case PixelType::QUADBAYER_RGGB:
        case PixelType::QUADBAYER_BGGR:
        case PixelType::QUADBAYER_GRBG:
        case PixelType::QUADBAYER_GBRG:
            return 1;
        case PixelType::GRAY_ALPHA:
            return 2;
        case PixelType::RGB:
        case PixelType::YUV:
            return 3;
        case PixelType::RGBA:
            return 4;
        case PixelType::CUSTOM:
            break;
    }
    return 0;
}

/// Checks whether the given pixel type is bayer.
inline bool isBayerPixelType(PixelType pixelType) {
    return pixelType == PixelType::BAYER_RGGB || pixelType == PixelType::BAYER_BGGR ||
           pixelType == PixelType::BAYER_GRBG || pixelType == PixelType::BAYER_GBRG;
}

/// Checks whether the given pixel type is quad bayer.
inline bool isQuadBayerPixelType(PixelType pixelType) {
    return pixelType == PixelType::QUADBAYER_RGGB || pixelType == PixelType::QUADBAYER_BGGR ||
           pixelType == PixelType::QUADBAYER_GRBG || pixelType == PixelType::QUADBAYER_GBRG;
}

/// Returns the X offset of a bayer color for a given bayer phase.
inline int bayerOffsetX(PixelType pixelType, Bayer bayer) {
    using namespace std::string_literals;

    switch (pixelType) {
        case PixelType::BAYER_RGGB:
        case PixelType::BAYER_GBRG:
            switch (bayer) {
                case Bayer::R:
                case Bayer::GB:
                    return 0;
                case Bayer::GR:
                case Bayer::B:
                    return 1;
            }
            break;
        case PixelType::BAYER_BGGR:
        case PixelType::BAYER_GRBG:
            switch (bayer) {
                case Bayer::R:
                case Bayer::GB:
                    return 1;
                case Bayer::GR:
                case Bayer::B:
                    return 0;
            }
            break;
        default:
            break;
    }
    throw std::invalid_argument("Invalid pixel type "s + toString(pixelType));
}

/// Returns the Y offset of a bayer color for a given bayer phase.
inline int bayerOffsetY(PixelType pixelType, Bayer bayer) {
    using namespace std::string_literals;

    switch (pixelType) {
        case PixelType::BAYER_RGGB:
        case PixelType::BAYER_GRBG:
            switch (bayer) {
                case Bayer::R:
                case Bayer::GR:
                    return 0;
                case Bayer::GB:
                case Bayer::B:
                    return 1;
            }
            break;
        case PixelType::BAYER_BGGR:
        case PixelType::BAYER_GBRG:
            switch (bayer) {
                case Bayer::R:
                case Bayer::GR:
                    return 1;
                case Bayer::GB:
                case Bayer::B:
                    return 0;
            }
            break;
        default:
            break;
    }
    throw std::invalid_argument("Invalid pixel type "s + toString(pixelType));
}

} // namespace model

} // namespace cxximg
