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

#include "cxximg/io/ImageReader.h"
#include "cxximg/io/ImageWriter.h"

#include "cxximg/util/File.h"

namespace cxximg {

#pragma pack(push, 1)
struct BmpHeader final {
    // BMP file header
    uint16_t signature;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offsetData;

    // BMP information header
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitPerPixel;
    uint32_t compressionMethod;
    uint32_t imageSize;
    int32_t hPixelsPerMeter;
    int32_t vPixelPerMeter;
    uint32_t colorsInPalette;
    uint32_t importantColors;
};
#pragma pack(pop)

static_assert(sizeof(BmpHeader) == 54, "BmpHeader must by 54 bytes");

class BmpReader final : public ImageReader {
public:
    static bool accept(const std::string &path, const uint8_t *signature, bool signatureValid) {
        if (!signatureValid) {
            return file::extension(path) == "bmp";
        }
        return signature[0] == 'B' && signature[1] == 'M';
    }

    using ImageReader::ImageReader;

    void initialize() override;

    Image8u read8u() override;

private:
    bool mUpsideDown = false;
};

class BmpWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "bmp"; }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::RGB ||
               descriptor.pixelType == PixelType::RGBA;
    }

    void write(const Image8u &image) override;
};

} // namespace cxximg
