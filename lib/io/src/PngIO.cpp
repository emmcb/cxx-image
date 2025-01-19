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

#include "PngIO.h"

#include <png.h>
#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "PNG";

namespace {

void pngReadData(png_structp png, png_bytep data, png_size_t length) {
    auto *stream = static_cast<std::istream *>(png_get_io_ptr(png));
    stream->read(reinterpret_cast<char *>(data), length);

    if (stream->fail()) {
        png_error(png, "Read error");
    }
}

void pngWriteData(png_structp png, png_bytep data, png_size_t length) {
    auto *stream = static_cast<std::ostream *>(png_get_io_ptr(png));
    stream->write(reinterpret_cast<const char *>(data), length);

    if (stream->fail()) {
        png_error(png, "Write error");
    }
}

void pngFlushData(png_structp png) {
    auto *stream = static_cast<std::ostream *>(png_get_io_ptr(png));
    stream->flush();
}

} // namespace

void PngReadDeleter::operator()(png_struct *png) {
    if (info) {
        png_destroy_read_struct(&png, &info, nullptr);
    } else {
        png_destroy_read_struct(&png, nullptr, nullptr);
    }
}

static PixelType colorTypeToPixelType(int colorType) {
    switch (colorType) {
        case PNG_COLOR_TYPE_GRAY:
            return PixelType::GRAYSCALE;
        case PNG_COLOR_TYPE_GA:
            return PixelType::GRAY_ALPHA;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_PALETTE:
            return PixelType::RGB;
        case PNG_COLOR_TYPE_RGBA:
            return PixelType::RGBA;
        default:
            throw IOError(MODULE, "Unsupported color type " + std::to_string(colorType));
    }
}

static int pixelTypeToColorType(PixelType pixelType) {
    switch (pixelType) {
        case PixelType::GRAYSCALE:
            return PNG_COLOR_TYPE_GRAY;
        case PixelType::GRAY_ALPHA:
            return PNG_COLOR_TYPE_GA;
        case PixelType::RGB:
            return PNG_COLOR_TYPE_RGB;
        case PixelType::RGBA:
            return PNG_COLOR_TYPE_RGBA;
        default:
            throw IOError(MODULE, "Unsupported pixel type "s + toString(pixelType));
    }
}

void PngReader::readHeader() {
    mPng.reset(png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr));

    png_structp png = mPng.get();
    png_infop info = png_create_info_struct(png);

    mPng.get_deleter().info = info;

    // setjmp() must be called in every function that calls a PNG-writing libpng function
    if (setjmp(png_jmpbuf(png))) { // NOLINT(cert-err52-cpp)
        throw IOError(MODULE, "Reading failed");
    }

    png_set_read_fn(png, static_cast<png_voidp>(mStream), pngReadData);
    png_read_info(png, info); // read all PNG info up to image data

    uint32_t width = 0, height = 0;
    int bitDepth = 0, colorType = 0;
    png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);

    // expand palette images to RGB, low-bit-depth grayscale images to 8 bits, and transparency chunks to full alpha
    // channel
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_expand(png);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS) != 0) {
        png_set_expand(png);
    }

    // set up byte order in 16-bit depth files
    png_set_swap(png);

    // all transformations have been registered; now update info_ptr data
    png_read_update_info(png, info);

    PixelRepresentation pixelRepresentation;
    if (bitDepth <= 8) {
        pixelRepresentation = PixelRepresentation::UINT8;
    } else if (bitDepth == 16) {
        pixelRepresentation = PixelRepresentation::UINT16;
    } else {
        throw IOError(MODULE, "Unsupported bit depth " + std::to_string(bitDepth));
    }

    mDescriptor = {LayoutDescriptor::Builder(width, height)
                           .imageLayout(ImageLayout::INTERLEAVED)
                           .pixelType(colorTypeToPixelType(colorType))
                           .pixelPrecision(bitDepth)
                           .build(),
                   pixelRepresentation};
}

Image8u PngReader::read8u() {
    LOG_SCOPE_F(INFO, "Read PNG (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint8_t>();
}

Image16u PngReader::read16u() {
    LOG_SCOPE_F(INFO, "Read PNG (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

template <typename T>
Image<T> PngReader::read() {
    validateType<T>();

    png_structp png = mPng.get();

    // setjmp() must be called in every function that calls a PNG-writing libpng function
    if (setjmp(png_jmpbuf(png))) { // NOLINT(cert-err52-cpp)
        throw IOError(MODULE, "Reading failed");
    }

    // allocate image memory
    Image<T> image(layoutDescriptor());

    std::vector<png_bytep> rowPointers(image.height());

    for (int y = 0; y < image.height(); ++y) {
        rowPointers[y] = reinterpret_cast<png_bytep>(&image(0, y, 0));
    }

    // now we can go ahead and just read the whole image
    png_read_image(png, rowPointers.data());
    png_read_end(png, nullptr);

    return image;
}

void PngWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write PNG (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint8_t>(image);
}

void PngWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write PNG (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint16_t>(image);
}

template <typename T>
void PngWriter::writeImpl(const Image<T> &image) const {
    if (image.imageLayout() == ImageLayout::PLANAR && image.numPlanes() > 1) {
        // Planar to interleaved conversion
        return writeImpl<T>(image::convertLayout(image, ImageLayout::INTERLEAVED));
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);

    // setjmp() must be called in every function that calls a PNG-writing libpng function
    if (setjmp(png_jmpbuf(png))) { // NOLINT(cert-err52-cpp)
        png_destroy_write_struct(&png, &info);
        throw IOError(MODULE, "Writing failed");
    }

    png_set_write_fn(png, static_cast<png_voidp>(mStream), pngWriteData, pngFlushData);

    // set the compression levels
    png_set_compression_level(png, 3);

    // set the image parameters appropriately
    png_set_IHDR(png,
                 info,
                 image.width(),
                 image.height(),
                 8 * sizeof(T),
                 pixelTypeToColorType(image.pixelType()),
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    // write all chunks up to (but not including) first IDAT
    png_write_info(png, info);

    // set up the transformations: for now, just pack low-bit-depth pixels into bytes (one, two or four pixels per byte)
    png_set_packing(png);

    // set up byte order in 16-bit depth files
    png_set_swap(png);

    // and now we just write the whole image; libpng takes care of interlacing for us
    const int64_t rowStride = image.layoutDescriptor().planes[0].rowStride;
    T *imageData = image.descriptor().buffers[0];

    std::vector<png_bytep> rowPointers(image.height());
    for (int y = 0; y < image.height(); ++y) {
        rowPointers[y] = reinterpret_cast<png_bytep>(imageData + y * rowStride);
    }

    png_write_image(png, rowPointers.data());

    // since that's it, we also close out the end of the PNG file now--if we had any text or time info to write after
    // the IDATs, second argument would be info_ptr, but we optimize slightly by sending NULL pointer:
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
}

} // namespace cxximg
