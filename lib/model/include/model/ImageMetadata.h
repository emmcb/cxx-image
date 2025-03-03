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

#include "ExifMetadata.h"
#include "ImageLayout.h"
#include "PixelType.h"

#include "math/ColorSpace.h"
#include "math/DynamicMatrix.h"
#include "math/Matrix.h"
#include "math/Rect.h"

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

enum class FileFormat { PLAIN, RAW10, RAW12 };

enum class PixelRepresentation { UINT8, UINT16, FLOAT };

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

    struct FileInfo final {
        std::optional<FileFormat> fileFormat;                   ///< File format
        std::optional<PixelRepresentation> pixelRepresentation; ///< Pixel representation
        std::optional<ImageLayout> imageLayout;                 ///< Image layout
        std::optional<PixelType> pixelType;                     ///< Pixel type
        std::optional<uint8_t> pixelPrecision;                  ///< Bit precision of pixel
        std::optional<uint16_t> width;                          ///< Image width
        std::optional<uint16_t> height;                         ///< Image height
        std::optional<uint16_t> widthAlignment;                 ///< Width alignment (must be a power of 2).
        std::optional<uint16_t> heightAlignment;                ///< Height alignment (must be a power of 2).
        std::optional<uint16_t> sizeAlignment;                  ///< Buffer size alignment (must be a power of 2).
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

    FileInfo fileInfo;               ///< File Information
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

inline const char *toString(FileFormat fileFormat) {
    switch (fileFormat) {
        case FileFormat::PLAIN:
            return "plain";
        case FileFormat::RAW10:
            return "raw10";
        case FileFormat::RAW12:
            return "raw12";
    }
    return "undefined";
}

inline const char *toString(PixelRepresentation pixelRepresentation) {
    switch (pixelRepresentation) {
        case PixelRepresentation::UINT8:
            return "uint8";
        case PixelRepresentation::UINT16:
            return "uint16";
        case PixelRepresentation::FLOAT:
            return "float";
    }
    return "undefined";
}

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

inline std::optional<FileFormat> parseFileFormat(const std::string &fileFormat) {
    if (fileFormat == "plain") {
        return FileFormat::PLAIN;
    }
    if (fileFormat == "raw10") {
        return FileFormat::RAW10;
    }
    if (fileFormat == "raw12") {
        return FileFormat::RAW12;
    }
    return std::nullopt;
}

inline std::optional<PixelRepresentation> parsePixelRepresentation(const std::string &pixelRepresentation) {
    if (pixelRepresentation == "uint8") {
        return PixelRepresentation::UINT8;
    }
    if (pixelRepresentation == "uint16") {
        return PixelRepresentation::UINT16;
    }
    if (pixelRepresentation == "float") {
        return PixelRepresentation::FLOAT;
    }
    return std::nullopt;
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
