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

#include "JpegIO.h"

#include <turbojpeg.h>
#include <loguru.hpp>

#if defined(WITH_JPEG_EXIF)
#include <libexif/exif-data.h>
extern "C" {
#include <libjpeg/jpeg-data.h>
}
#endif

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "JPEG";

void JpegDeleter::operator()(void *handle) const {
    tjDestroy(handle);
}

JpegReader::JpegReader(const std::string &path, const Options &options) : ImageReader(path, options) {
    int width = 0;
    int height = 0;
    int jpegSubsamp = 0;
    int jpegColorspace = 0;
    int result = -1;

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw IOError(MODULE, "Cannot open input file for reading");
    }

    // 65K should be enough to read JPEG header in most case, however we have no guarantee about this. Particularly,
    // some manufacturers may add custom APP segments with proprietary data, which increases the header size.
    constexpr int CHUNK_SIZE = 65536;
    std::vector<uint8_t> chunk(CHUNK_SIZE);

    while (result != 0) {
        const int bytesRead = file.rdbuf()->sgetn(reinterpret_cast<char *>(chunk.data()), CHUNK_SIZE);
        if (bytesRead <= 0) {
            break;
        }
        mHeaderData.insert(mHeaderData.end(), chunk.begin(), chunk.begin() + bytesRead);

        mHandle.reset(tjInitDecompress());
        result = tjDecompressHeader3(
                mHandle.get(), mHeaderData.data(), mHeaderData.size(), &width, &height, &jpegSubsamp, &jpegColorspace);

        if (bytesRead < CHUNK_SIZE) {
            break;
        }
    }

    if (result != 0) {
        throw IOError(MODULE, "Failed to decompress header: "s + tjGetErrorStr2(mHandle.get()));
    }

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(width, height).pixelPrecision(8);

    if (jpegSubsamp == TJSAMP_GRAY) {
        builder.pixelType(PixelType::GRAYSCALE);
    } else if (options.jpegDecodingMode == ImageReader::JpegDecodingMode::RGB) {
        builder.imageLayout(ImageLayout::INTERLEAVED).pixelType(PixelType::RGB);
    } else if (jpegSubsamp == TJSAMP_444) {
        builder.pixelType(PixelType::YUV);
    } else if (jpegSubsamp == TJSAMP_420) {
        builder.imageLayout(ImageLayout::YUV_420);
    } else {
        throw IOError(MODULE, "Unsupported subsampling value " + std::to_string(jpegSubsamp));
    }

    setDescriptor({builder.build(), PixelRepresentation::UINT8});
}

Image8u JpegReader::read8u() {
    LOG_SCOPE_F(INFO, "Read JPEG");
    LOG_S(INFO) << "Path: " << path();

    // Read the binary file to a buffer
    std::vector<uint8_t> data = file::readBinary(path());

    Image8u image(layoutDescriptor());
    int result = -1;

    switch (image.pixelType()) {
        case PixelType::GRAYSCALE:
            result = tjDecompress2(mHandle.get(),
                                   data.data(),
                                   data.size(),
                                   image.data(),
                                   image.width(),
                                   0,
                                   image.height(),
                                   TJPF_GRAY,
                                   TJFLAG_ACCURATEDCT);
            break;

        case PixelType::RGB:
            result = tjDecompress2(mHandle.get(),
                                   data.data(),
                                   data.size(),
                                   image.data(),
                                   image.width(),
                                   0,
                                   image.height(),
                                   TJPF_RGB,
                                   TJFLAG_ACCURATEDCT);
            break;

        case PixelType::YUV:
            result = tjDecompressToYUV2(mHandle.get(),
                                        data.data(),
                                        data.size(),
                                        image.data(),
                                        image.width(),
                                        image.layoutDescriptor().widthAlignment,
                                        image.height(),
                                        TJFLAG_ACCURATEDCT);
            break;

        default:
            throw IOError(MODULE, "Unsupported pixel type "s + toString(image.pixelType()));
    }

    if (result != 0) {
        throw IOError(MODULE, "Failed to decompress data: "s + tjGetErrorStr2(mHandle.get()));
    }

    return image;
}

#if defined(WITH_JPEG_EXIF)
std::optional<ExifMetadata> JpegReader::readExif() const {
    ExifData *data = exif_data_new_from_data(mHeaderData.data(), mHeaderData.size());
    if (data == nullptr) {
        return std::nullopt;
    }

    ExifMetadata exif;

    ExifContent *ifd0 = data->ifd[EXIF_IFD_0];
    exif_content_foreach_entry(
            ifd0,
            [](ExifEntry *entry, void *userData) {
                const ExifByteOrder byteOrder = exif_data_get_byte_order(entry->parent->parent);
                auto *exif = static_cast<ExifMetadata *>(userData);

                switch (entry->tag) {
                    case EXIF_TAG_IMAGE_WIDTH:
                        exif->imageWidth = exif_get_short(entry->data, byteOrder);
                        break;
                    case EXIF_TAG_IMAGE_LENGTH:
                        exif->imageHeight = exif_get_short(entry->data, byteOrder);
                        break;
                    case EXIF_TAG_IMAGE_DESCRIPTION:
                        exif->imageDescription = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    case EXIF_TAG_MAKE:
                        exif->make = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    case EXIF_TAG_MODEL:
                        exif->model = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    case EXIF_TAG_ORIENTATION:
                        exif->orientation = exif_get_short(entry->data, byteOrder);
                        break;
                    case EXIF_TAG_SOFTWARE:
                        exif->software = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    default:
                        break;
                }
            },
            &exif);

    ExifContent *ifdExif = data->ifd[EXIF_IFD_EXIF];
    exif_content_foreach_entry(
            ifdExif,
            [](ExifEntry *entry, void *userData) {
                const ExifByteOrder byteOrder = exif_data_get_byte_order(entry->parent->parent);
                auto *exif = static_cast<ExifMetadata *>(userData);
                ExifRational rational;

                switch (entry->tag) {
                    case EXIF_TAG_EXPOSURE_TIME:
                        rational = exif_get_rational(entry->data, byteOrder);
                        exif->exposureTime = {rational.numerator, rational.denominator};
                        break;
                    case EXIF_TAG_FNUMBER:
                        rational = exif_get_rational(entry->data, byteOrder);
                        exif->fNumber = {rational.numerator, rational.denominator};
                        break;
                    case EXIF_TAG_ISO_SPEED_RATINGS:
                        exif->isoSpeedRatings = exif_get_short(entry->data, byteOrder);
                        break;
                    case EXIF_TAG_DATE_TIME_ORIGINAL:
                        exif->dateTimeOriginal = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    case EXIF_TAG_FOCAL_LENGTH:
                        rational = exif_get_rational(entry->data, byteOrder);
                        exif->focalLength = {rational.numerator, rational.denominator};
                        break;
                    case EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM:
                        exif->focalLengthIn35mmFilm = exif_get_short(entry->data, byteOrder);
                        break;
                    default:
                        break;
                }
            },
            &exif);

    exif_data_unref(data);

    return exif;
}
#endif

#if defined(WITH_JPEG_EXIF)
static ExifEntry *addExifEntry(ExifContent *ifd, ExifTag tag) {
    ExifEntry *entry = exif_content_get_entry(ifd, tag);
    if (entry) {
        return entry;
    }

    entry = exif_entry_new();
    exif_content_add_entry(ifd, entry);
    exif_entry_initialize(entry, tag);
    exif_entry_unref(entry);

    return entry;
}

static void exifSetString(ExifMem *mem, ExifEntry *entry, const std::string &str) {
    if (entry->data) {
        exif_mem_free(mem, entry->data);
    }

    entry->components = str.size();
    entry->size = exif_format_get_size(entry->format) * entry->components;
    entry->data = reinterpret_cast<unsigned char *>(exif_mem_alloc(mem, entry->size));

    str.copy(reinterpret_cast<char *>(entry->data), entry->size);
}

static void populateExif(ExifMem *mem, ExifData *data, ExifMetadata exif) {
    constexpr ExifByteOrder FILE_BYTE_ORDER = EXIF_BYTE_ORDER_MOTOROLA;

    exif_data_set_option(data, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
    exif_data_set_data_type(data, EXIF_DATA_TYPE_COMPRESSED);
    exif_data_set_byte_order(data, FILE_BYTE_ORDER);
    exif_data_fix(data);

    ExifContent *ifd0 = data->ifd[EXIF_IFD_0];
    ExifContent *ifdExif = data->ifd[EXIF_IFD_EXIF];
    ExifEntry *entry = nullptr;

    addExifEntry(ifd0, EXIF_TAG_DATE_TIME); // initialized by libexif to current date/time

    entry = addExifEntry(ifdExif, EXIF_TAG_COLOR_SPACE);
    exif_set_short(entry->data, FILE_BYTE_ORDER, 1);

    if (exif.imageWidth) {
        entry = addExifEntry(ifdExif, EXIF_TAG_PIXEL_X_DIMENSION);
        exif_set_long(entry->data, FILE_BYTE_ORDER, *exif.imageWidth);

        entry = addExifEntry(ifd0, EXIF_TAG_IMAGE_WIDTH);
        exif_set_short(entry->data, FILE_BYTE_ORDER, *exif.imageWidth);
    }
    if (exif.imageHeight) {
        entry = addExifEntry(ifdExif, EXIF_TAG_PIXEL_Y_DIMENSION);
        exif_set_long(entry->data, FILE_BYTE_ORDER, *exif.imageHeight);

        entry = addExifEntry(ifd0, EXIF_TAG_IMAGE_LENGTH);
        exif_set_short(entry->data, FILE_BYTE_ORDER, *exif.imageHeight);
    }
    if (exif.imageDescription) {
        entry = addExifEntry(ifd0, EXIF_TAG_IMAGE_DESCRIPTION);
        exifSetString(mem, entry, *exif.imageDescription);
    }
    if (exif.make) {
        entry = addExifEntry(ifd0, EXIF_TAG_MAKE);
        exifSetString(mem, entry, *exif.make);
    }
    if (exif.model) {
        entry = addExifEntry(ifd0, EXIF_TAG_MODEL);
        exifSetString(mem, entry, *exif.model);
    }
    if (exif.orientation) {
        entry = addExifEntry(ifd0, EXIF_TAG_ORIENTATION);
        exif_set_short(entry->data, FILE_BYTE_ORDER, *exif.orientation);
    }
    if (exif.software) {
        entry = addExifEntry(ifd0, EXIF_TAG_SOFTWARE);
        exifSetString(mem, entry, *exif.software);
    }
    if (exif.exposureTime) {
        entry = addExifEntry(ifdExif, EXIF_TAG_EXPOSURE_TIME);
        exif_set_rational(entry->data, FILE_BYTE_ORDER, {exif.exposureTime->numerator, exif.exposureTime->denominator});
    }
    if (exif.fNumber) {
        entry = addExifEntry(ifdExif, EXIF_TAG_FNUMBER);
        exif_set_rational(entry->data, FILE_BYTE_ORDER, {exif.fNumber->numerator, exif.fNumber->denominator});
    }
    if (exif.isoSpeedRatings) {
        entry = addExifEntry(ifdExif, EXIF_TAG_ISO_SPEED_RATINGS);
        exif_set_short(entry->data, FILE_BYTE_ORDER, *exif.isoSpeedRatings);
    }
    if (exif.dateTimeOriginal) {
        entry = addExifEntry(ifdExif, EXIF_TAG_DATE_TIME_ORIGINAL);
        exifSetString(mem, entry, *exif.dateTimeOriginal);
    }
    if (exif.focalLength) {
        entry = addExifEntry(ifdExif, EXIF_TAG_FOCAL_LENGTH);
        exif_set_rational(entry->data, FILE_BYTE_ORDER, {exif.focalLength->numerator, exif.focalLength->denominator});
    }
    if (exif.focalLengthIn35mmFilm) {
        entry = addExifEntry(ifdExif, EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM);
        exif_set_short(entry->data, FILE_BYTE_ORDER, *exif.focalLengthIn35mmFilm);
    }
}
#endif

void JpegWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write JPEG");
    LOG_S(INFO) << "Path: " << path();

    std::unique_ptr<void, JpegDeleter> handle(tjInitCompress());

    // Compress image buffer to a compressed buffer.
    // Memory is allocated by tjCompress2 if jpegSize == 0.
    unsigned char *jpegBuf = nullptr;
    uint64_t jpegSize = 0;

    const auto compress = [&](const Image8u &interleavedImage) {
        const bool isGrayscale = image.pixelType() == PixelType::GRAYSCALE;
        const int pixelFormat = isGrayscale ? TJPF_GRAY : TJPF_RGB;
        const int jpegSubsamp = isGrayscale ? TJSAMP_GRAY : TJSAMP_420;

        return tjCompress2(handle.get(),
                           interleavedImage.data(),
                           interleavedImage.width(),
                           0,
                           interleavedImage.height(),
                           pixelFormat,
                           &jpegBuf,
                           &jpegSize,
                           jpegSubsamp,
                           options().jpegQuality,
                           TJFLAG_ACCURATEDCT);
    };

    const auto compressYUV = [&](const Image8u &yuvImage) {
        const bool isYuv444 = yuvImage.imageLayout() == ImageLayout::PLANAR && yuvImage.pixelType() == PixelType::YUV;
        const int jpegSubsamp = isYuv444 ? TJSAMP_444 : TJSAMP_420;

        return tjCompressFromYUV(handle.get(),
                                 yuvImage.data(),
                                 yuvImage.width(),
                                 yuvImage.layoutDescriptor().widthAlignment,
                                 yuvImage.height(),
                                 jpegSubsamp,
                                 &jpegBuf,
                                 &jpegSize,
                                 options().jpegQuality,
                                 TJFLAG_ACCURATEDCT);
    };

    int result = -1;
    switch (image.pixelType()) {
        case PixelType::GRAYSCALE:
            // No conversion needed
            result = compress(image);
            break;

        case PixelType::RGB:
            switch (image.imageLayout()) {
                case ImageLayout::INTERLEAVED:
                    // No conversion needed
                    result = compress(image);
                    break;
                case ImageLayout::PLANAR:
                    // Planar to interleaved conversion
                    result = compress(image::convertLayout(image, ImageLayout::INTERLEAVED));
                    break;
                default:
                    throw IOError(MODULE,
                                  "Unsupported image layout "s + toString(image.imageLayout()) + " for pixel type " +
                                          toString(image.pixelType()));
            }
            break;

        case PixelType::YUV:
            switch (image.imageLayout()) {
                case ImageLayout::PLANAR:
                case ImageLayout::YUV_420:
                    // No conversion needed
                    result = compressYUV(image);
                    break;
                case ImageLayout::NV12:
                    // NV12 to YUV_420 conversion
                    result = compressYUV(image::convertLayout(image, ImageLayout::YUV_420));
                    break;
                default:
                    throw IOError(MODULE,
                                  "Unsupported image layout "s + toString(image.imageLayout()) + " for pixel type " +
                                          toString(image.pixelType()));
            }
            break;

        default:
            throw IOError(MODULE, "Unsupported pixel type "s + toString(image.pixelType()));
    }

    if (result != 0) {
        throw IOError(MODULE, "Failed to compress data: "s + tjGetErrorStr2(handle.get()));
    }

#if defined(WITH_JPEG_EXIF)
    // Append exif if given
    const auto &metadata = options().metadata;
    if (metadata) {
        ExifMem *mem = exif_mem_new_default();
        ExifData *data = exif_data_new();

        populateExif(mem, data, metadata->exifMetadata);

        JPEGData *jpeg = jpeg_data_new_from_data(jpegBuf, jpegSize);
        jpeg_data_set_exif_data(jpeg, data);
        if (jpeg_data_save_file(jpeg, path().c_str()) == 0) {
            throw IOError(MODULE, "Cannot save file");
        }
        jpeg_data_unref(jpeg);

        exif_mem_unref(mem);
        exif_data_unref(data);
    }

    // Else directly write the compressed buffer to file
    else {
#endif
        std::ofstream file(path(), std::ios::binary);
        if (!file) {
            throw IOError(MODULE, "Cannot open output file for writing");
        }

        file.write(reinterpret_cast<char *>(jpegBuf), jpegSize);
#if defined(WITH_JPEG_EXIF)
    }
#endif
}

#if defined(WITH_JPEG_EXIF)
void JpegWriter::writeExif(const ExifMetadata &exif) const {
    ExifMem *mem = exif_mem_new_default();
    ExifData *data = exif_data_new();

    populateExif(mem, data, exif);

    JPEGData *jpeg = jpeg_data_new_from_file(path().c_str());
    jpeg_data_set_exif_data(jpeg, data);
    if (jpeg_data_save_file(jpeg, path().c_str()) == 0) {
        throw IOError(MODULE, "Cannot open output file for writing");
    }
    jpeg_data_unref(jpeg);

    exif_mem_unref(mem);
    exif_data_unref(data);
}
#endif

} // namespace cxximg
