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

#include "ExifMetadata.h"

#include "cxximg/math/ColorSpace.h"
#include "cxximg/math/DynamicMatrix.h"
#include "cxximg/math/Matrix.h"
#include "cxximg/math/Rect.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cxximg {

/// @defgroup model Model library

/// @addtogroup model
/// @{

enum class SemanticLabel { NONE, PERSON, SKIN, SKY, UNKNOWN };

/// Structure holding image metadata.
struct ImageMetadata final {
    struct ColorShading final {
        DynamicMatrix gainR; ///< Color lens shading R/G correction map
        DynamicMatrix gainB; ///< Color lens shading B/G correction map
    };

    struct WhiteBalance final {
        float gainR; ///< White balance R/G scale
        float gainB; ///< White balance B/G scale
    };

    struct SemanticMask final {
        std::string name;    ///< Identification string
        SemanticLabel label; ///< Semantic label
        DynamicMatrix mask;  ///< Semantic mask
    };

    struct CameraControls final {
        std::optional<WhiteBalance> whiteBalance;        ///< White balance scales
        std::optional<ColorShading> colorShading;        ///< Color lens shading correction maps
        std::optional<std::vector<Rectf>> faceDetection; ///< Array of face ROI (using normalized coordinates)
    };

    struct ShootingParams final {
        std::optional<float> aperture;     ///< Aperture
        std::optional<float> exposureTime; ///< Exposure time
        std::optional<float> sensitivity;  ///< Standard ISO sensitivity
        std::optional<float> totalGain;    ///< Total applied gain (= sensorGain * ispgain)
        std::optional<float> sensorGain;   ///< Sensor gain
        std::optional<float> ispGain;      ///< ISP gain
        std::optional<Rectf> zoom;         ///< Zoom ROI (using normalized coordinates)
    };

    struct CalibrationData final {
        std::optional<std::variant<int, float>> blackLevel; ///< Black level
        std::optional<std::variant<int, float>> whiteLevel; ///< White level
        std::optional<DynamicMatrix> vignetting;            ///< Luminance lens shading correction map
        std::optional<Matrix3> colorMatrix;                 ///< Color matrix
        std::optional<RgbColorSpace> colorMatrixTarget;     ///< Target color space of color matrix
    };

    using SemanticMasks = std::unordered_multimap<SemanticLabel, SemanticMask>;

    ExifMetadata exifMetadata;       ///< Exif metadata
    ShootingParams shootingParams;   ///< Shooting params
    CalibrationData calibrationData; ///< Calibration data
    CameraControls cameraControls;   ///< Camera controls
    SemanticMasks semanticMasks;     ///< Semantic masks

    void synchronize() {
        // Initialize shooting params from EXIF

        if (!shootingParams.aperture && exifMetadata.fNumber) {
            shootingParams.aperture = exifMetadata.fNumber->asFloat();
        }
        if (!shootingParams.exposureTime && exifMetadata.exposureTime) {
            shootingParams.exposureTime = exifMetadata.exposureTime->asFloat();
        }
        if (!shootingParams.sensitivity && exifMetadata.isoSpeedRatings) {
            shootingParams.sensitivity = *exifMetadata.isoSpeedRatings;
        }

        // TODO: initialize EXIF from shooting params
    }
};

inline const char *toString(SemanticLabel semanticLabel) {
    switch (semanticLabel) {
        case SemanticLabel::NONE:
            return "none";
        case SemanticLabel::PERSON:
            return "person";
        case SemanticLabel::SKIN:
            return "skin";
        case SemanticLabel::SKY:
            return "sky";
        case SemanticLabel::UNKNOWN:
            return "unknown";
    }
    return "undefined";
}

inline std::optional<SemanticLabel> parseSemanticLabel(const std::string &semanticLabel) {
    if (semanticLabel == "none") {
        return SemanticLabel::NONE;
    }
    if (semanticLabel == "person") {
        return SemanticLabel::PERSON;
    }
    if (semanticLabel == "skin") {
        return SemanticLabel::SKIN;
    }
    if (semanticLabel == "sky") {
        return SemanticLabel::SKY;
    }
    if (semanticLabel == "unknown") {
        return SemanticLabel::UNKNOWN;
    }
    return std::nullopt;
}

/// @}

} // namespace cxximg
