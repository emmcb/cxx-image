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

// NOLINTBEGIN
// from png.h
typedef struct png_struct_def png_struct;
typedef struct png_info_def png_info;
// NOLINTEND

namespace cxximg {

struct PngReadDeleter final {
    void operator()(png_struct *png) const;
};

struct PngInfoDeleter final {
    png_struct *png = nullptr;
    void operator()(png_info *info) const;
};

class PngReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        std::vector<uint8_t> header = file::readBinary(path, 8);
        return header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E && header[3] == 0x47 && header[4] == 0x0D &&
               header[5] == 0x0A && header[6] == 0x1A && header[7] == 0x0A;
    }

    PngReader(const std::string &path, const Options &options);

    Image8u read8u() override;
    Image16u read16u() override;

private:
    template <typename T>
    Image<T> read();

    std::unique_ptr<FILE, FileDeleter> mFile;
    std::unique_ptr<png_struct, PngReadDeleter> mPng;
    std::unique_ptr<png_info, PngInfoDeleter> mInfo;
};

class PngWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "png"; }

    PngWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::GRAY_ALPHA ||
               descriptor.pixelType == PixelType::RGB || descriptor.pixelType == PixelType::RGBA;
    }

    void write(const Image8u &image) const override;
    void write(const Image16u &image) const override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
