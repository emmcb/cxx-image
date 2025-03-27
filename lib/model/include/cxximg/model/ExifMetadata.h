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

#include <cstdint>
#include <optional>
#include <string>

namespace cxximg {

/// Structure holding EXIF metadata.
/// @ingroup model
struct ExifMetadata final {
    struct Rational final {
        uint32_t numerator = 0;
        uint32_t denominator = 1;

        inline float asFloat() const noexcept { return static_cast<float>(numerator) / denominator; }
        inline double asDouble() const noexcept { return static_cast<double>(numerator) / denominator; }
    };

    struct SRational final {
        int32_t numerator = 0;
        int32_t denominator = 1;

        inline float asFloat() const noexcept { return static_cast<float>(numerator) / denominator; }
        inline double asDouble() const noexcept { return static_cast<double>(numerator) / denominator; }
    };

    std::optional<uint16_t> imageWidth;            ///< Image width reported in EXIF data
    std::optional<uint16_t> imageHeight;           ///< Image height reported in EXIF data
    std::optional<std::string> imageDescription;   ///< Image description
    std::optional<std::string> make;               ///< Camera manufacturer's name
    std::optional<std::string> model;              ///< Camera model
    std::optional<uint16_t> orientation;           ///< Image orientation
    std::optional<std::string> software;           ///< Software used
    std::optional<Rational> exposureTime;          ///< Exposure time in seconds
    std::optional<Rational> fNumber;               ///< F/stop
    std::optional<uint16_t> isoSpeedRatings;       ///< ISO speed
    std::optional<std::string> dateTimeOriginal;   ///< Date when original image was taken
    std::optional<SRational> brightnessValue;      ///< The value of brightness.
    std::optional<SRational> exposureBiasValue;    ///< The exposure bias.
    std::optional<Rational> focalLength;           ///< Focal length of lens in millimeters
    std::optional<uint16_t> focalLengthIn35mmFilm; ///< Focal length of lens in millimeters (35mm equivalent)
    std::optional<std::string> lensMake;           ///< Lens manufacturer
    std::optional<std::string> lensModel;          ///< Lens model
};

} // namespace cxximg
