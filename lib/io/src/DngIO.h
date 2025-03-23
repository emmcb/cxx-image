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

#include "cxximg/io/ImageReader.h"
#include "cxximg/io/ImageWriter.h"

#include "cxximg/util/File.h"

class dng_stream;
class dng_host;
class dng_info;
class dng_negative;

namespace cxximg {

class DngReader final : public ImageReader {
public:
    static bool accept(const std::string &path, const uint8_t *signature, bool signatureValid) {
        if (!signatureValid) {
            return file::extension(path) == "dng";
        }

        // Same header than TIFF, thus we should also check the extension
        return ((signature[0] == 'I' && signature[1] == 'I' && signature[2] == 0x2a && signature[3] == 0) ||
                (signature[0] == 'M' && signature[1] == 'M' && signature[2] == 0 && signature[3] == 0x2a)) &&
               file::extension(path) == "dng";
    }

    DngReader(const std::string &path, std::istream *stream, const Options &options);
    ~DngReader() override;

    void readHeader() override;

    Image16u read16u() override;
    Imagef readf() override;

    std::optional<ExifMetadata> readExif() const override;
    void readMetadata(std::optional<ImageMetadata> &metadata) const override;

private:
    template <typename T>
    Image<T> read();

    std::unique_ptr<dng_stream> mStream;
    std::unique_ptr<dng_host> mHost;
    std::unique_ptr<dng_info> mInfo;
    std::unique_ptr<dng_negative> mNegative;
};

class DngWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) {
        std::string ext = file::extension(path);
        return ext == "dng";
    }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return model::isBayerPixelType(descriptor.pixelType) || model::isQuadBayerPixelType(descriptor.pixelType) ||
               descriptor.pixelType == PixelType::RGB;
    }

    void write(const Image16u &image) const override;
    void write(const Imagef &image) const override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
