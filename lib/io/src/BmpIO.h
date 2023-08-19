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

#include "io/ImageReader.h"
#include "io/ImageWriter.h"

#include "util/File.h"

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
    static bool accept(const std::string &path) {
        std::vector<uint8_t> header = file::readBinary(path, 2);
        return header[0] == 0x42 && header[1] == 0x4d;
    }

    BmpReader(const std::string &path, const Options &options);

    Image8u read8u() override;
};

class BmpWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "bmp"; }

    BmpWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::RGB ||
               descriptor.pixelType == PixelType::RGBA;
    }

    void write(const Image8u &image) const override;
};

} // namespace cxximg
