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
// from tiffio.h
typedef struct tiff TIFF;
// NOLINTEND

namespace cxximg {

struct TiffDeleter final {
    void operator()(TIFF *tif) const;
};

using TiffPtr = std::unique_ptr<TIFF, TiffDeleter>;

class TiffReader final : public ImageReader {
public:
    static bool accept(const std::string &path, const uint8_t *signature, bool signatureValid) {
        if (!signatureValid) {
            const std::string ext = file::extension(path);
            return ext == "tiff" || ext == "tif";
        }

        // Bytes 0-1 should be 'II' or 'MM', 'II' means little endian, 'MM' means big endian, and bytes 2-3 should be
        // magic number 42.
        return (signature[0] == 'I' && signature[1] == 'I' && signature[2] == 0x2a && signature[3] == 0) ||
               (signature[0] == 'M' && signature[1] == 'M' && signature[2] == 0 && signature[3] == 0x2a);
    }

    using ImageReader::ImageReader;

    void readHeader() override;

    Image8u read8u() override;
    Image16u read16u() override;
    Imagef readf() override;

    std::optional<ExifMetadata> readExif() const override;

private:
    template <typename T>
    Image<T> read();

    TiffPtr mTiff;
};

class TiffWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) {
        const std::string ext = file::extension(path);
        return ext == "tiff" || ext == "tif";
    }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::RGB ||
               model::isBayerPixelType(descriptor.pixelType) || model::isQuadBayerPixelType(descriptor.pixelType);
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
