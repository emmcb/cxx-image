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

#include "MipiRawIO.h"
#include "Alignment.h"

#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "MIPIRAW";

Raw10Pixel &Raw10Pixel::operator=(const Raw16From10Pixel &pixel) {
    p1234 = ((pixel.p4 & 3) << 6) | ((pixel.p3 & 3) << 4) | ((pixel.p2 & 3) << 2) | (pixel.p1 & 3);
    p1 = pixel.p1 >> 2;
    p2 = pixel.p2 >> 2;
    p3 = pixel.p3 >> 2;
    p4 = pixel.p4 >> 2;
    return *this;
}

Raw12Pixel &Raw12Pixel::operator=(const Raw16From12Pixel &pixel) {
    p12 = ((pixel.p2 & 15) << 4) | (pixel.p1 & 15);
    p1 = pixel.p1 >> 4;
    p2 = pixel.p2 >> 4;
    return *this;
}

Raw16From10Pixel &Raw16From10Pixel::operator=(const Raw10Pixel &pixel) {
    p1 = (pixel.p1 << 2) | (pixel.p1234 & 3);
    p2 = (pixel.p2 << 2) | ((pixel.p1234 >> 2) & 3);
    p3 = (pixel.p3 << 2) | ((pixel.p1234 >> 4) & 3);
    p4 = (pixel.p4 << 2) | ((pixel.p1234 >> 6) & 3);
    return *this;
}

Raw16From12Pixel &Raw16From12Pixel::operator=(const Raw12Pixel &pixel) {
    p1 = (pixel.p1 << 4) | (pixel.p12 & 15);
    p2 = (pixel.p2 << 4) | (pixel.p12 >> 4);
    return *this;
}

template <int PIXEL_PRECISION, class RawXPixel, class Raw16FromXPixel>
void MipiRawReader<PIXEL_PRECISION, RawXPixel, Raw16FromXPixel>::readHeader() {
    const auto &fileInfo = options().fileInfo;
    if (!fileInfo.width || !fileInfo.height) {
        throw IOError(MODULE, "Unspecified image dimensions");
    }
    if (!fileInfo.pixelType) {
        throw IOError(MODULE, "Unspecified pixel type");
    }
    if (!image::isBayerPixelType(*fileInfo.pixelType) && !image::isQuadBayerPixelType(*fileInfo.pixelType)) {
        throw IOError(MODULE, "Pixel type must be bayer (got"s + toString(*fileInfo.pixelType) + ")");
    }
    if ((*fileInfo.width * PIXEL_PRECISION) % 8 != 0) {
        throw IOError(MODULE,
                      "Invalid image width for MIPIRAW" + std::to_string(PIXEL_PRECISION) +
                              " format: " + std::to_string(*fileInfo.width));
    }

    mDescriptor = {LayoutDescriptor::Builder(*fileInfo.width, *fileInfo.height)
                           .pixelType(*fileInfo.pixelType)
                           .pixelPrecision(PIXEL_PRECISION)
                           .build(),
                   PixelRepresentation::UINT16};
}

template <int PIXEL_PRECISION, class RawXPixel, class Raw16FromXPixel>
Image16u MipiRawReader<PIXEL_PRECISION, RawXPixel, Raw16FromXPixel>::read16u() {
    LOG_SCOPE_F(INFO, "Read MIPIRAW%d", PIXEL_PRECISION);
    LOG_S(INFO) << "Path: " << path();

    mStream->seekg(0, std::istream::end);
    int64_t fileSize = mStream->tellg();
    mStream->seekg(0);

    std::vector<uint8_t> data(fileSize);
    mStream->read(reinterpret_cast<char *>(data.data()), data.size());

    LayoutDescriptor descriptor = layoutDescriptor();
    LayoutDescriptor::Builder packedBuilder = LayoutDescriptor::Builder(descriptor.width * PIXEL_PRECISION / 8,
                                                                        descriptor.height)
                                                      .numPlanes(1);
    packedBuilder.widthAlignment([&]() -> int {
        if (options().fileInfo.widthAlignment) {
            return *options().fileInfo.widthAlignment;
        }

        // Look for the width alignment that matches with the file size.
        std::optional<int> widthAlignment = detail::guessWidthAlignment(packedBuilder, data.size());
        if (!widthAlignment) {
            throw IOError(
                    MODULE,
                    "Cannot guess relevant width alignment corresponding to file size " + std::to_string(data.size()));
        }

        LOG_S(INFO) << "Guess width alignment " << *widthAlignment << " from file size " << data.size() << ".";
        return *widthAlignment;
    }());

    LayoutDescriptor packedDescriptor = packedBuilder.build();
    if (static_cast<int64_t>(data.size()) != packedDescriptor.requiredBufferSize()) {
        throw IOError(MODULE,
                      "File size does not match specified MIPIRAW" + std::to_string(PIXEL_PRECISION) +
                              " image dimension (expected " + std::to_string(packedDescriptor.requiredBufferSize()) +
                              ", got " + std::to_string(data.size()) + ")");
    }

    const auto unpack = [&](uint8_t *packedData) {
        Image16u image(descriptor);

        ImageView<RawXPixel> rawXImage(
                ImageDescriptor<RawXPixel>(
                        LayoutDescriptor::Builder(descriptor.width / (sizeof(RawXPixel) - 1), descriptor.height)
                                .numPlanes(1)
                                .build())
                        .map(reinterpret_cast<RawXPixel *>(packedData)));

        ImageView<Raw16FromXPixel> raw16Image(
                ImageDescriptor<Raw16FromXPixel>(
                        LayoutDescriptor::Builder(descriptor.width / (sizeof(RawXPixel) - 1), descriptor.height)
                                .numPlanes(1)
                                .build())
                        .map(reinterpret_cast<Raw16FromXPixel *>(image.data())));

        // Unpack MIPIRAW
        raw16Image = rawXImage;

        return image;
    };

    if (packedDescriptor.widthAlignment == 1) {
        // Source image does not have any padding bytes.
        return unpack(data.data());
    }

    // Source image has padding bytes at the end of each rows, remove them.
    ImageView8u packedImage(ImageDescriptor8u(packedDescriptor).map(data.data()));
    Image8u unalignedImage(LayoutDescriptor::Builder(packedBuilder).widthAlignment(1).build(), packedImage);

    return unpack(unalignedImage.data());
}

template <int PIXEL_PRECISION, class RawXPixel, class Raw16FromXPixel>
void MipiRawWriter<PIXEL_PRECISION, RawXPixel, Raw16FromXPixel>::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write MIPIRAW%d", PIXEL_PRECISION);
    LOG_S(INFO) << "Path: " << path();

    std::ofstream stream(path(), std::ios::binary);
    if (!stream) {
        throw IOError("Cannot open file for writing: " + path());
    }

    if (image.pixelPrecision() != PIXEL_PRECISION) {
        throw IOError(MODULE,
                      "Invalid pixel precision for MIPIRAW" + std::to_string(PIXEL_PRECISION) +
                              " format: " + std::to_string(image.pixelPrecision()));
    }

    if ((image.width() * PIXEL_PRECISION) % 8 != 0) {
        throw IOError(MODULE,
                      "Invalid image width for MIPIRAW" + std::to_string(PIXEL_PRECISION) +
                              " format: " + std::to_string(image.width()));
    }

    Image8u packedImage(
            LayoutDescriptor::Builder(image.width() * PIXEL_PRECISION / 8, image.height()).numPlanes(1).build());

    ImageView<Raw16FromXPixel> raw16Image(
            ImageDescriptor<Raw16FromXPixel>(
                    LayoutDescriptor::Builder(image.width() / (sizeof(RawXPixel) - 1), image.height())
                            .numPlanes(1)
                            .build())
                    .map(const_cast<Raw16FromXPixel *>(reinterpret_cast<const Raw16FromXPixel *>(image.data()))));

    ImageView<RawXPixel> rawXImage(
            ImageDescriptor<RawXPixel>(
                    LayoutDescriptor::Builder(image.width() / (sizeof(RawXPixel) - 1), image.height())
                            .numPlanes(1)
                            .build())
                    .map(reinterpret_cast<RawXPixel *>(packedImage.data())));

    // Pack to MIPIRAW
    rawXImage = raw16Image;

    stream.write(reinterpret_cast<const char *>(packedImage.data()), packedImage.size());
}

template class MipiRawReader<10, Raw10Pixel, Raw16From10Pixel>;
template class MipiRawReader<12, Raw12Pixel, Raw16From12Pixel>;
template class MipiRawWriter<10, Raw10Pixel, Raw16From10Pixel>;
template class MipiRawWriter<12, Raw12Pixel, Raw16From12Pixel>;

} // namespace cxximg
