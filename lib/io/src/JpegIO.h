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

struct JpegDeleter final {
    void operator()(void *handle) const;
};

class JpegReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        std::vector<uint8_t> header = file::readBinary(path, 4);
        return header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF && (header[3] == 0xE1 || header[3] == 0xE0);
    }

    JpegReader(const std::string &path, const Options &options);

    Image8u read8u() override;

#ifdef HAVE_EXIF
    std::optional<ExifMetadata> readExif() const override;
#endif

private:
    std::unique_ptr<void, JpegDeleter> mHandle;
    std::vector<uint8_t> mHeaderData;
};

class JpegWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) {
        std::string ext = file::extension(path);
        return ext == "jpeg" || ext == "jpg";
    }

    JpegWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return descriptor.pixelType == PixelType::GRAYSCALE || descriptor.pixelType == PixelType::RGB ||
               descriptor.pixelType == PixelType::YUV;
    }

    void write(const Image8u &image) const override;

#ifdef HAVE_EXIF
    void writeExif(const ExifMetadata &exif) const override;
#endif
};

} // namespace cxximg
