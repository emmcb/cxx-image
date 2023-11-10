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

BmpReader::BmpReader(const std::string &path, const Options &options) : ImageReader(path, options) {
    std::vector<uint8_t> data = file::readBinary(path, sizeof(BmpHeader));
    const auto *header = reinterpret_cast<BmpHeader *>(data.data());

    if (header->headerSize < 40) {
        throw IOError(MODULE,
                      "Unsupported header size (expected at least 40, got " + std::to_string(header->headerSize) + ")");
    }
    if (header->compressionMethod != 0) {
        throw IOError(MODULE, "Unsupported compression method (only uncompressed is supported)");
    }

    setDescriptor({LayoutDescriptor::Builder(header->width, std::abs(header->height))
                           .imageLayout(ImageLayout::INTERLEAVED)
                           .pixelType(bppToPixelType(header->bitPerPixel))
                           .pixelPrecision(8)
                           .build(),
                   PixelRepresentation::UINT8});
}

Image8u BmpReader::read8u() {
    LOG_SCOPE_F(INFO, "Read BMP");
    LOG_S(INFO) << "Path: " << path();

    std::vector<uint8_t> data = file::readBinary(path());
    const auto *header = reinterpret_cast<BmpHeader *>(data.data());

    LayoutDescriptor descriptor = layoutDescriptor();
    LayoutDescriptor alignedDescriptor = LayoutDescriptor::Builder(descriptor).widthAlignment(4).build();
    if (static_cast<int64_t>(data.size()) != header->offsetData + alignedDescriptor.requiredBufferSize()) {
        throw IOError(MODULE,
                      "File size does not match expected buffer size (expected " +
                              std::to_string(header->offsetData + alignedDescriptor.requiredBufferSize()) + ", got " +
                              std::to_string(data.size()) + ")");
    }

    auto *pixelArray = reinterpret_cast<uint8_t *>(data.data() + header->offsetData);
    ImageView8u alignedImage(ImageDescriptor8u(alignedDescriptor).map(pixelArray));

    // ABGR to RGBA conversion, without alignment
    Image8u image(descriptor);
    for (auto dstPlane : image.planes()) {
        const auto srcPlane = alignedImage.plane(alignedImage.numPlanes() - dstPlane.index() - 1);
        if (header->height > 0) {
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

    std::ofstream file(path(), std::ios::binary);
    if (!file) {
        throw IOError(MODULE, "Cannot open output file for writing");
    }

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(alignedImage.data()), alignedImage.size());
}

} // namespace cxximg
