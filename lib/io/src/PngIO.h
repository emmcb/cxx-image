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

// NOLINTBEGIN
// from png.h
typedef struct png_struct_def png_struct;
typedef struct png_info_def png_info;
// NOLINTEND

namespace cxximg {

struct PngReadDeleter final {
    png_info *info = nullptr;
    void operator()(png_struct *png);
};

class PngReader final : public ImageReader {
public:
    static bool accept(const std::string &path, const uint8_t *signature, bool signatureValid) {
        if (!signatureValid) {
            return file::extension(path) == "png";
        }
        return signature[0] == 0x89 && signature[1] == 0x50 && signature[2] == 0x4E && signature[3] == 0x47 &&
               signature[4] == 0x0D && signature[5] == 0x0A && signature[6] == 0x1A && signature[7] == 0x0A;
    }

    using ImageReader::ImageReader;

    void initialize() override;

    Image8u read8u() override;
    Image16u read16u() override;

private:
    template <typename T>
    Image<T> read();

    std::unique_ptr<png_struct, PngReadDeleter> mPng;
};

class PngWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "png"; }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::GRAY_ALPHA ||
               descriptor.pixelType == PixelType::RGB || descriptor.pixelType == PixelType::RGBA;
    }

    void write(const Image8u &image) override;
    void write(const Image16u &image) override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image);
};

} // namespace cxximg
