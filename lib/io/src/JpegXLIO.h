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

// from jxl/decode.h
typedef struct JxlDecoderStruct JxlDecoder; // NOLINT

namespace cxximg {

struct JxlDecoderDeleter final {
    void operator()(JxlDecoder *decoder) const;
};

class JpegXLReader final : public ImageReader {
public:
    static bool accept(const std::string &path, const uint8_t *signature, bool signatureValid) {
        if (!signatureValid) {
            return file::extension(path) == "jxl";
        }
        return (signature[0] == 0xFF && signature[1] == 0x0A) ||
               (signature[0] == 0 && signature[1] == 0 && signature[2] == 0 && signature[3] == 0x0C &&
                signature[4] == 'J' && signature[5] == 'X' && signature[6] == 'L' && signature[7] == ' ' &&
                signature[8] == 0x0D && signature[9] == 0x0A && signature[10] == 0x87 && signature[11] == 0x0A);
    }

    using ImageReader::ImageReader;

    void initialize() override;

    Image8u read8u() override;
    Image16u read16u() override;
    Imagef readf() override;

#ifdef HAVE_EXIF
    std::optional<ExifMetadata> readExif() const override;
#endif

private:
    static constexpr int CHUNK_SIZE = 65536;

    template <typename T>
    Image<T> read();

    std::unique_ptr<JxlDecoder, JxlDecoderDeleter> mDecoder;

    std::array<uint8_t, CHUNK_SIZE> mBuffer;
    size_t mRemainingBytes = 0;

    std::vector<uint8_t> mExif;
};

class JpegXLWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "jxl"; }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::GRAY_ALPHA ||
               descriptor.pixelType == PixelType::RGB || descriptor.pixelType == PixelType::RGBA;
    }

    void write(const Image8u &image) const override;
    void write(const Image16u &image) const override;
    void write(const Imagef &image) const override;

private:
    static constexpr int CHUNK_SIZE = 65536;

    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
