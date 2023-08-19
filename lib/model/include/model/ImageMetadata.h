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

#include "image/ImageLayout.h"
#include "image/PixelType.h"
#include "math/ColorSpace.h"
#include "math/DynamicMatrix.h"
#include "math/Matrix.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace cxximg {

/// @defgroup model Model library

/// @addtogroup model
/// @{

enum class FileFormat { PLAIN, RAW10, RAW12 };

enum class PixelRepresentation { UINT8, UINT16, FLOAT };

/// Structure holding image metadata.
struct ImageMetadata final {
    struct ColorLensShading final {
        DynamicMatrix gainR; ///< Color lens shading R/G correction map
        DynamicMatrix gainB; ///< Color lens shading B/G correction map
    };

    struct WhiteBalance final {
        float gainR; ///< White balance R/G scale
        float gainB; ///< White balance B/G scale
    };

    struct ROI final {
        float x;      ///< x for ROI
        float y;      ///< y for ROI
        float width;  ///< width for ROI
        float height; ///< height for ROI
    };

    struct FileInfo final {
        std::optional<FileFormat> fileFormat;                   ///< File format
        std::optional<PixelRepresentation> pixelRepresentation; ///< Pixel representation
        std::optional<ImageLayout> imageLayout;                 ///< Image layout
        std::optional<PixelType> pixelType;                     ///< Pixel type
        std::optional<uint8_t> pixelPrecision;                  ///< Bit precision of pixel
        std::optional<uint16_t> width;                          ///< Image width
        std::optional<uint16_t> height;                         ///< Image height
        std::optional<uint8_t> widthAlignment;                  ///< Width alignment
    };

    struct CameraControls final {
        std::optional<WhiteBalance> whiteBalance;         ///< White balance scales
        std::optional<ColorLensShading> colorLensShading; ///< Color lens shading correction maps
        std::optional<std::vector<ROI>> faceDetection;    ///< Array of face ROI
    };

    struct ShootingParams final {
        std::optional<float> aperture;     ///< Aperture
        std::optional<float> exposureTime; ///< Exposure time
        std::optional<float> totalGain;    ///< Total gain
        std::optional<float> sensorGain;   ///< Sensor gain
        std::optional<float> ispGain;      ///< ISP gain
    };

    struct TuningData final {
        std::optional<std::variant<int, float>> blackLevel; ///< Black level
        std::optional<std::variant<int, float>> whiteLevel; ///< White level
        std::optional<DynamicMatrix> luminanceLensShading;  ///< Luminance lens shading correction map
        std::optional<Matrix3> colorMatrix;                 ///< Color matrix
        std::optional<RgbColorSpace> colorMatrixTarget;     ///< Target color space of color matrix
    };

    FileInfo fileInfo;             ///< File Information
    ExifMetadata exifMetadata;     ///< Exif metadata
    CameraControls cameraControls; ///< Camera controls
    ShootingParams shootingParams; ///< Shooting params
    TuningData tuningData;         ///< Tuning data
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

/// @}

} // namespace cxximg
