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

#include "ImageReaderWriter.h"
#include "JsonDto.h"

#include "cxximg/model/ImageMetadata.h"

namespace cxximg {

namespace detail {

// Rect struct

template <typename T>
void read_json_value(Rect<T>& roi, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() != 4) {
        throw json_dto::ex_t("Invalid ROI value");
    }
    json_dto::read_json_value(roi.x, object[0]);
    json_dto::read_json_value(roi.y, object[1]);
    json_dto::read_json_value(roi.width, object[2]);
    json_dto::read_json_value(roi.height, object[3]);
}

template <typename T>
void write_json_value(const Rect<T>& roi, rapidjson::Value& object, rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.PushBack(roi.x, allocator);
    object.PushBack(roi.y, allocator);
    object.PushBack(roi.width, allocator);
    object.PushBack(roi.height, allocator);
}

} // namespace detail

// FileFormat enum

inline void read_json_value(FileFormat& fileFormat, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parseFileFormat(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid file format " + representation);
    }
    fileFormat = *parsed;
}

inline void write_json_value(const FileFormat& fileFormat,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(fileFormat)), object, allocator);
}

// PixelRepresentation enum

inline void read_json_value(PixelRepresentation& pixelRepresentation, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parsePixelRepresentation(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid pixel representation " + representation);
    }
    pixelRepresentation = *parsed;
}

inline void write_json_value(const PixelRepresentation& pixelRepresentation,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(pixelRepresentation)), object, allocator);
}

// ImageLayout enum

inline void read_json_value(ImageLayout& imageLayout, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parseImageLayout(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid image layout " + representation);
    }
    imageLayout = *parsed;
}

inline void write_json_value(const ImageLayout& imageLayout,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(imageLayout)), object, allocator);
}

// PixelType enum

inline void read_json_value(PixelType& pixelType, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parsePixelType(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid pixel type " + representation);
    }
    pixelType = *parsed;
}

inline void write_json_value(const PixelType& pixelType,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(pixelType)), object, allocator);
}

// RgbColorSpace enum

inline void read_json_value(RgbColorSpace& rgbColorSpace, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parseRgbColorSpace(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid RGB color space " + representation);
    }
    rgbColorSpace = *parsed;
}

inline void write_json_value(const RgbColorSpace& rgbColorSpace,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(rgbColorSpace)), object, allocator);
}

// SemanticLabel enum

inline void read_json_value(SemanticLabel& semanticLabel, const rapidjson::Value& object) {
    std::string representation;
    json_dto::read_json_value(representation, object);

    const auto parsed = parseSemanticLabel(representation);
    if (!parsed) {
        throw json_dto::ex_t("Invalid semantic label " + representation);
    }
    semanticLabel = *parsed;
}

inline void write_json_value(const SemanticLabel& semanticLabel,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    json_dto::write_json_value(json_dto::make_string_ref(toString(semanticLabel)), object, allocator);
}

// DynamicMatrix class

inline void read_json_value(DynamicMatrix& matrix, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() == 0) {
        throw json_dto::ex_t("Invalid matrix value");
    }
    matrix = DynamicMatrix(object.Size(), object[0].Size());
    for (int i = 0; i < matrix.numRows(); ++i) {
        const auto& row = object[i];
        if (!row.IsArray() || static_cast<int>(row.Size()) != matrix.numCols()) {
            throw json_dto::ex_t("Invalid matrix value");
        }
        for (int j = 0; j < matrix.numCols(); ++j) {
            json_dto::read_json_value(matrix(i, j), row[j]);
        }
    }
}

inline void write_json_value(const DynamicMatrix& matrix,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.Reserve(matrix.numRows(), allocator);
    for (int i = 0; i < matrix.numRows(); ++i) {
        rapidjson::Value row(rapidjson::kArrayType);
        row.Reserve(matrix.numCols(), allocator);
        for (int j = 0; j < matrix.numCols(); ++j) {
            row.PushBack(matrix(i, j), allocator);
        }
        object.PushBack(row, allocator);
    }
}

// Matrix<M, N> class

template <int M, int N>
void read_json_value(Matrix<M, N>& matrix, const rapidjson::Value& object) {
    if (!object.IsArray() || static_cast<int>(object.Size()) != M) {
        throw json_dto::ex_t("Invalid matrix value");
    }
    for (int i = 0; i < M; ++i) {
        const auto& row = object[i];
        if (!row.IsArray() || static_cast<int>(row.Size()) != N) {
            throw json_dto::ex_t("Invalid matrix value");
        }
        for (int j = 0; j < N; ++j) {
            json_dto::read_json_value(matrix(i, j), row[j]);
        }
    }
}

template <int M, int N>
void write_json_value(const Matrix<M, N>& matrix,
                      rapidjson::Value& object,
                      rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.Reserve(M, allocator);
    for (int i = 0; i < M; ++i) {
        rapidjson::Value row(rapidjson::kArrayType);
        row.Reserve(N, allocator);
        for (int j = 0; j < N; ++j) {
            row.PushBack(matrix(i, j), allocator);
        }
        object.PushBack(row, allocator);
    }
}

// ExifMetadata::Rational struct

inline void read_json_value(ExifMetadata::Rational& rational, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() != 2) {
        throw json_dto::ex_t("Invalid EXIF rational value");
    }
    json_dto::read_json_value(rational.numerator, object[0]);
    json_dto::read_json_value(rational.denominator, object[1]);
}

inline void write_json_value(const ExifMetadata::Rational& rational,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.PushBack(rational.numerator, allocator);
    object.PushBack(rational.denominator, allocator);
}

// ExifMetadata::SRational struct

inline void read_json_value(ExifMetadata::SRational& rational, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() != 2) {
        throw json_dto::ex_t("Invalid EXIF rational value");
    }
    json_dto::read_json_value(rational.numerator, object[0]);
    json_dto::read_json_value(rational.denominator, object[1]);
}

inline void write_json_value(const ExifMetadata::SRational& rational,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.PushBack(rational.numerator, allocator);
    object.PushBack(rational.denominator, allocator);
}

// ImageMetadata::ColorShading struct

inline void read_json_value(ImageMetadata::ColorShading& colorShading, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() != 2) {
        throw json_dto::ex_t("Invalid color lens shading value");
    }
    read_json_value(colorShading.gainR, object[0]);
    read_json_value(colorShading.gainB, object[1]);
}

inline void write_json_value(const ImageMetadata::ColorShading& colorShading,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    rapidjson::Value rValue, bValue;
    write_json_value(colorShading.gainR, rValue, allocator);
    write_json_value(colorShading.gainB, bValue, allocator);

    object.SetArray();
    object.PushBack(rValue, allocator);
    object.PushBack(bValue, allocator);
}

// ImageMetadata::WhiteBalance struct

inline void read_json_value(ImageMetadata::WhiteBalance& whiteBalance, const rapidjson::Value& object) {
    if (!object.IsArray() || object.Size() != 2) {
        throw json_dto::ex_t("Invalid white balance value");
    }
    json_dto::read_json_value(whiteBalance.gainR, object[0]);
    json_dto::read_json_value(whiteBalance.gainB, object[1]);
}

inline void write_json_value(const ImageMetadata::WhiteBalance& whiteBalance,
                             rapidjson::Value& object,
                             rapidjson::MemoryPoolAllocator<>& allocator) {
    object.SetArray();
    object.PushBack(whiteBalance.gainR, allocator);
    object.PushBack(whiteBalance.gainB, allocator);
}

struct SemanticMasksReaderWriter final {
    static void read(ImageMetadata::SemanticMasks& semanticMasks, const rapidjson::Value& object) {
        std::vector<ImageMetadata::SemanticMask> semanticMasksList;
        json_dto::read_json_value(semanticMasksList, object);

        for (auto& semanticMask : semanticMasksList) {
            semanticMasks.emplace(semanticMask.label, std::move(semanticMask));
        }
    }

    static void write(const ImageMetadata::SemanticMasks& semanticMasks,
                      rapidjson::Value& object,
                      rapidjson::MemoryPoolAllocator<>& allocator) {
        std::vector<ImageMetadata::SemanticMask> semanticMasksList;
        for (const auto& [label, semanticMask] : semanticMasks) {
            semanticMasksList.push_back(semanticMask);
        }

        json_dto::write_json_value(semanticMasksList, object, allocator);
    }
};

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata::SemanticMask& semanticMask) {
    io& json_dto::mandatory("label", semanticMask.label) &
            json_dto::mandatory(ImageLoader{}, "file", semanticMask.mask);
}

template <typename JsonIo>
void json_io(JsonIo& io, ExifMetadata& exifMetadata) {
    io& json_dto::optional("imageWidth", exifMetadata.imageWidth, std::nullopt) &
            json_dto::optional("imageHeight", exifMetadata.imageHeight, std::nullopt) &
            json_dto::optional("imageDescription", exifMetadata.imageDescription, std::nullopt) &
            json_dto::optional("make", exifMetadata.make, std::nullopt) &
            json_dto::optional("model", exifMetadata.model, std::nullopt) &
            json_dto::optional("orientation", exifMetadata.orientation, std::nullopt) &
            json_dto::optional("software", exifMetadata.software, std::nullopt) &
            json_dto::optional("exposureTime", exifMetadata.exposureTime, std::nullopt) &
            json_dto::optional("fNumber", exifMetadata.fNumber, std::nullopt) &
            json_dto::optional("isoSpeedRatings", exifMetadata.isoSpeedRatings, std::nullopt) &
            json_dto::optional("dateTimeOriginal", exifMetadata.dateTimeOriginal, std::nullopt) &
            json_dto::optional("brightnessValue", exifMetadata.brightnessValue, std::nullopt) &
            json_dto::optional("exposureBiasValue", exifMetadata.exposureBiasValue, std::nullopt) &
            json_dto::optional("focalLength", exifMetadata.focalLength, std::nullopt) &
            json_dto::optional("focalLengthIn35mmFilm", exifMetadata.focalLengthIn35mmFilm, std::nullopt);
}

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata::FileInfo& fileInfo) {
    io& json_dto::optional("fileFormat", fileInfo.fileFormat, std::nullopt) &
            json_dto::optional("pixelRepresentation", fileInfo.pixelRepresentation, std::nullopt) &
            json_dto::optional("imageLayout", fileInfo.imageLayout, std::nullopt) &
            json_dto::optional("pixelType", fileInfo.pixelType, std::nullopt) &
            json_dto::optional("pixelPrecision", fileInfo.pixelPrecision, std::nullopt) &
            json_dto::optional("width", fileInfo.width, std::nullopt) &
            json_dto::optional("height", fileInfo.height, std::nullopt) &
            json_dto::optional("widthAlignment", fileInfo.widthAlignment, std::nullopt) &
            json_dto::optional("heightAlignment", fileInfo.heightAlignment, std::nullopt) &
            json_dto::optional("sizeAlignment", fileInfo.sizeAlignment, std::nullopt);
}

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata::CameraControls& cameraControls) {
    io& json_dto::optional("whiteBalance", cameraControls.whiteBalance, std::nullopt) &
            json_dto::optional("colorShading", cameraControls.colorShading, std::nullopt) &
            json_dto::optional("faceDetection", cameraControls.faceDetection, std::nullopt);
}

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata::ShootingParams& shootingParams) {
    io& json_dto::optional("aperture", shootingParams.aperture, std::nullopt) &
            json_dto::optional("exposureTime", shootingParams.exposureTime, std::nullopt) &
            json_dto::optional("sensitivity", shootingParams.sensitivity, std::nullopt) &
            json_dto::optional("totalGain", shootingParams.totalGain, std::nullopt) &
            json_dto::optional("sensorGain", shootingParams.sensorGain, std::nullopt) &
            json_dto::optional("ispGain", shootingParams.ispGain, std::nullopt) &
            json_dto::optional("zoom", shootingParams.zoom, std::nullopt);
}

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata::CalibrationData& calibrationData) {
    io& json_dto::optional("blackLevel", calibrationData.blackLevel, std::nullopt) &
            json_dto::optional("whiteLevel", calibrationData.whiteLevel, std::nullopt) &
            json_dto::optional("vignetting", calibrationData.vignetting, std::nullopt) &
            json_dto::optional("colorMatrix", calibrationData.colorMatrix, std::nullopt) &
            json_dto::optional("colorMatrixTarget", calibrationData.colorMatrixTarget, std::nullopt);
}

template <typename JsonIo>
void json_io(JsonIo& io, ImageMetadata& metadata) {
    io& json_dto::optional_no_default("fileInfo", metadata.fileInfo) &
            json_dto::optional_no_default("exifMetadata", metadata.exifMetadata) &
            json_dto::optional_no_default("shootingParams", metadata.shootingParams) &
            json_dto::optional_no_default("calibrationData", metadata.calibrationData) &
            json_dto::optional_no_default("cameraControls", metadata.cameraControls) &
            json_dto::optional_no_default(SemanticMasksReaderWriter{}, "semanticMasks", metadata.semanticMasks);
}

} // namespace cxximg
