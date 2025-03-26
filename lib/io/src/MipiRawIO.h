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

struct Raw16From10Pixel;
struct Raw16From12Pixel;

struct Raw10Pixel final {
    uint8_t p1;
    uint8_t p2;
    uint8_t p3;
    uint8_t p4;
    uint8_t p1234;

    Raw10Pixel &operator=(const Raw16From10Pixel &pixel);
};

struct Raw12Pixel final {
    uint8_t p1;
    uint8_t p2;
    uint8_t p12;

    Raw12Pixel &operator=(const Raw16From12Pixel &pixel);
};

struct Raw16From10Pixel final {
    uint16_t p1;
    uint16_t p2;
    uint16_t p3;
    uint16_t p4;

    Raw16From10Pixel &operator=(const Raw10Pixel &pixel);
};

struct Raw16From12Pixel final {
    uint16_t p1;
    uint16_t p2;

    Raw16From12Pixel &operator=(const Raw12Pixel &pixel);
};

static_assert(sizeof(Raw10Pixel) == 5, "Raw10Pixel must by 5 bytes");
static_assert(sizeof(Raw12Pixel) == 3, "Raw12Pixel must by 3 bytes");
static_assert(sizeof(Raw16From10Pixel) == 8, "Raw16From10Pixel must by 8 bytes");
static_assert(sizeof(Raw16From12Pixel) == 4, "Raw16From12Pixel must by 4 bytes");

template <int PIXEL_PRECISION, class RawXPixel, class Raw16FromXPixel>
class MipiRawReader : public ImageReader {
public:
    using ImageReader::ImageReader;

    void initialize() override;

    Image16u read16u() override;
};

class MipiRaw10Reader final : public MipiRawReader<10, Raw10Pixel, Raw16From10Pixel> {
public:
    using MipiRawReader::MipiRawReader;

    static bool accept(const std::string &path) {
        std::string ext = file::extension(path);
        return ext == "rawmipi" || ext == "rawmipi10";
    }
};

class MipiRaw12Reader final : public MipiRawReader<12, Raw12Pixel, Raw16From12Pixel> {
public:
    using MipiRawReader::MipiRawReader;

    static bool accept(const std::string &path) { return file::extension(path) == "rawmipi12"; }
};

template <int PIXEL_PRECISION, class RawXPixel, class Raw16FromXPixel>
class MipiRawWriter : public ImageWriter {
public:
    using ImageWriter::ImageWriter;

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return model::isBayerPixelType(descriptor.pixelType);
    }

    void write(const Image16u &image) const override;
};

class MipiRaw10Writer final : public MipiRawWriter<10, Raw10Pixel, Raw16From10Pixel> {
public:
    using MipiRawWriter::MipiRawWriter;

    static bool accept(const std::string &path) {
        std::string ext = file::extension(path);
        return ext == "rawmipi" || ext == "rawmipi10";
    }
};

class MipiRaw12Writer final : public MipiRawWriter<12, Raw12Pixel, Raw16From12Pixel> {
public:
    using MipiRawWriter::MipiRawWriter;

    static bool accept(const std::string &path) { return file::extension(path) == "rawmipi12"; }
};

extern template class MipiRawReader<10, Raw10Pixel, Raw16From10Pixel>;
extern template class MipiRawReader<12, Raw12Pixel, Raw16From12Pixel>;
extern template class MipiRawWriter<10, Raw10Pixel, Raw16From10Pixel>;
extern template class MipiRawWriter<12, Raw12Pixel, Raw16From12Pixel>;

} // namespace cxximg
