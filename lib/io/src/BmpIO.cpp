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

#include "BmpIO.h"

#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "BMP";

static PixelType bppToPixelType(uint16_t bitPerPixel) {
    switch (bitPerPixel) {
        case 8:
            return PixelType::GRAYSCALE;
        case 24:
            return PixelType::RGB;
        case 32:
            return PixelType::RGBA;
        default:
            throw IOError(MODULE, "Unsupported bit per pixel " + std::to_string(bitPerPixel));
    }
}

static uint16_t pixelTypeToBPP(PixelType pixelType) {
    switch (pixelType) {
        case PixelType::GRAYSCALE:
            return 8;
        case PixelType::RGB:
            return 24;
        case PixelType::RGBA:
            return 32;
        default:
            throw IOError(MODULE, "Unsupported pixel type "s + toString(pixelType));
    }
}

void BmpReader::readHeader() {
    BmpHeader header = {};
    mStream->read(reinterpret_cast<char *>(&header), sizeof(header));

    if (mStream->fail()) {
        throw IOError(MODULE, "Failed to read header");
    }
    if (header.headerSize < 40) {
        throw IOError(MODULE,
                      "Unsupported header size (expected at least 40, got " + std::to_string(header.headerSize) + ")");
    }
    if (header.compressionMethod != 0) {
        throw IOError(MODULE, "Unsupported compression method (only uncompressed is supported)");
    }

    mDescriptor = {LayoutDescriptor::Builder(header.width, std::abs(header.height))
                           .imageLayout(ImageLayout::INTERLEAVED)
                           .pixelType(bppToPixelType(header.bitPerPixel))
                           .pixelPrecision(8)
                           .build(),
                   PixelRepresentation::UINT8};
    mUpsideDown = header.height > 0;

    mStream->seekg(header.offsetData);
}

Image8u BmpReader::read8u() {
    LOG_SCOPE_F(INFO, "Read BMP");
    LOG_S(INFO) << "Path: " << path();

    Image8u alignedImage(LayoutDescriptor::Builder(layoutDescriptor()).widthAlignment(4).build());

    int64_t curPos = mStream->tellg();
    mStream->seekg(0, std::istream::end);
    int64_t endPos = mStream->tellg();

    if (endPos - curPos != alignedImage.size()) {
        throw IOError(MODULE,
                      "File size does not match expected buffer size (expected " + std::to_string(alignedImage.size()) +
                              ", got " + std::to_string(endPos - curPos) + ")");
    }

    mStream->seekg(curPos);
    mStream->read(reinterpret_cast<char *>(alignedImage.data()), alignedImage.size());

    // ABGR to RGBA conversion, without alignment
    Image8u image(layoutDescriptor());
    for (auto dstPlane : image.planes()) {
        const auto srcPlane = alignedImage.plane(alignedImage.numPlanes() - dstPlane.index() - 1);
        if (mUpsideDown) {
            // BMP is bottom to top, we need to convert to top to bottom
            dstPlane = [&](int x, int y)
                               UTIL_ALWAYS_INLINE { return expr::evaluate(srcPlane, x, srcPlane.height() - y - 1); };
        } else {
            dstPlane = srcPlane;
        }
    }

    return image;
}

void BmpWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write BMP");
    LOG_S(INFO) << "Path: " << path();

    BmpHeader header = {.signature = 0x4D42, // BMP magic number
                        .fileSize = 0,
                        .reserved1 = 0,
                        .reserved2 = 0,
                        .offsetData = sizeof(BmpHeader),
                        .headerSize = 40,
                        .width = image.width(),
                        .height = -image.height(), // negative height is top to bottom
                        .planes = 1,
                        .bitPerPixel = pixelTypeToBPP(image.pixelType()),
                        .compressionMethod = 0,
                        .imageSize = 0,
                        .hPixelsPerMeter = 0,
                        .vPixelPerMeter = 0,
                        .colorsInPalette = 0,
                        .importantColors = 0};

    // RGBA to interleaved ABGR conversion, aligned to 4 bytes
    Image8u alignedImage(LayoutDescriptor::Builder(image.layoutDescriptor())
                                 .imageLayout(ImageLayout::INTERLEAVED)
                                 .widthAlignment(4)
                                 .build());
    for (auto plane : alignedImage.planes()) {
        plane = image.plane(image.numPlanes() - plane.index() - 1);
    }

    mStream->write(reinterpret_cast<const char *>(&header), sizeof(header));
    mStream->write(reinterpret_cast<const char *>(alignedImage.data()), alignedImage.size());
}

} // namespace cxximg
