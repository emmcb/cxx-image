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
// from tiffio.h
typedef struct tiff TIFF;
// NOLINTEND

namespace cxximg {

struct TiffDeleter final {
    void operator()(TIFF *tif) const;
};

class TiffReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        // Bytes 0-1 should be 'II' or 'MM', 'II' means little endian, 'MM' means
        // big endian, and Byte 2-3 should be magic number 42.
        std::vector<uint8_t> header = file::readBinary(path, 4);
        return (header[0] == 'I' && header[1] == 'I' && header[2] == 42 && header[3] == 0) ||
               (header[0] == 'M' && header[1] == 'M' && header[2] == 0 && header[3] == 42);
    }

    TiffReader(const std::string &path, const Options &options);

    Image8u read8u() override;
    Image16u read16u() override;
    Imagef readf() override;

    std::optional<ExifMetadata> readExif() const override;

private:
    template <typename T>
    Image<T> read();

    std::unique_ptr<TIFF, TiffDeleter> mTiff;
};

class TiffWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) {
        std::string ext = file::extension(path);
        return ext == "tiff" || ext == "tif";
    }

    TiffWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::RGB ||
               image::isBayerPixelType(descriptor.pixelType) || image::isQuadBayerPixelType(descriptor.pixelType);
    }

    void write(const Image8u &image) const override;
    void write(const Image16u &image) const override;
    void write(const Imagef &image) const override;

    void writeExif(const ExifMetadata &exif) const override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
