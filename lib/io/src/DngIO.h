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

class dng_file_stream;
class dng_host;
class dng_info;
class dng_negative;

namespace cxximg {

class DngReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        // Same header than TIFF, thus we should also check the extension
        std::vector<uint8_t> header = file::readBinary(path, 4);
        return ((header[0] == 'I' && header[1] == 'I' && header[2] == 42 && header[3] == 0) ||
                (header[0] == 'M' && header[1] == 'M' && header[2] == 0 && header[3] == 42)) &&
               file::extension(path) == "dng";
    }

    DngReader(const std::string &path, const Options &options);
    ~DngReader() override;

    Image16u read16u() override;
    Imagef readf() override;

    std::optional<ExifMetadata> readExif() const override;
    void updateMetadata(std::optional<ImageMetadata> &metadata) const override;

private:
    template <typename T>
    Image<T> read();

    std::unique_ptr<dng_file_stream> mStream;
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

    DngWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return image::isBayerPixelType(descriptor.pixelType) || image::isQuadBayerPixelType(descriptor.pixelType) ||
               descriptor.pixelType == PixelType::RGB;
    }

    void write(const Image16u &image) const override;
    void write(const Imagef &image) const override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
