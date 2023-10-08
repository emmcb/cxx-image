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

#include "TiffIO.h"

#include <tiffrational.h>
#include <loguru.hpp>
#include <tiffio.hxx>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "TIFF";

void TiffDeleter::operator()(TIFF *tif) const {
    TIFFClose(tif);
}

static PixelType cfaPatternToPixelType(const uint8_t *cfaPattern) {
    if (cfaPattern[0] == 0 && cfaPattern[1] == 1) {
        return PixelType::BAYER_RGGB;
    }
    if (cfaPattern[0] == 1 && cfaPattern[1] == 0) {
        return PixelType::BAYER_GRBG;
    }
    if (cfaPattern[0] == 2 && cfaPattern[1] == 1) {
        return PixelType::BAYER_BGGR;
    }
    if (cfaPattern[0] == 1 && cfaPattern[1] == 2) {
        return PixelType::BAYER_GBRG;
    }
    throw IOError(MODULE,
                  "Unsupported CFA pattern " + std::to_string(cfaPattern[0]) + " " + std::to_string(cfaPattern[1]));
}

TiffReader::TiffReader(const std::string &path, const Options &options)
    : ImageReader(path, options), mTiff(TIFFOpen(path.c_str(), "r")) {
    if (!mTiff) {
        throw IOError(MODULE, "Cannot open input file for reading");
    }
    TIFF *tif = mTiff.get();

    uint32_t width = 0;
    if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) == 0) {
        throw IOError(MODULE, "Failed to get TIFFTAG_IMAGEWIDTH");
    }
    uint32_t height = 0;
    if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) == 0) {
        throw IOError(MODULE, "Failed to get TIFFTAG_IMAGELENGTH");
    }
    uint16_t samplesPerPixel = 0;
    if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel) == 0) {
        throw IOError(MODULE, "Failed to get TIFFTAG_SAMPLESPERPIXEL");
    }

    uint16_t bitsPerSample = 0;
    if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample) == 0) {
        throw IOError(MODULE, "Failed to get TIFFTAG_BITSPERSAMPLE");
    }

    uint16_t sampleFormat = 0;
    if (TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat) == 0) {
        sampleFormat = SAMPLEFORMAT_UINT;
    }

    uint16_t photoMetric = 0;
    if (TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photoMetric) == 0) {
        throw IOError(MODULE, "Failed to get TIFFTAG_PHOTOMETRIC");
    }

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(width, height);

    if (samplesPerPixel == 1) {
        switch (photoMetric) {
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_MINISBLACK:
                builder.pixelType(PixelType::GRAYSCALE);
                break;
            case PHOTOMETRIC_CFA: {
                uint16_t count = 0;
                uint8_t *cfaPattern = nullptr;
                if (TIFFGetField(tif, TIFFTAG_CFAPATTERN, &count, &cfaPattern) != 0) {
                    builder.pixelType(cfaPatternToPixelType(cfaPattern));
                } else if (options.fileInfo.pixelType && (image::isBayerPixelType(*options.fileInfo.pixelType) ||
                                                          image::isQuadBayerPixelType(*options.fileInfo.pixelType))) {
                    builder.pixelType(*options.fileInfo.pixelType);
                } else {
                    throw IOError(MODULE, "Unspecified CFA pattern");
                }
            } break;
            default:
                throw IOError(MODULE, "Unsupported photo metric " + std::to_string(photoMetric));
        }
    } else if (samplesPerPixel == 3) {
        if (photoMetric != PHOTOMETRIC_RGB) {
            throw IOError(MODULE, "Unsupported photo metric " + std::to_string(photoMetric));
        }

        builder.pixelType(PixelType::RGB);

        uint16_t planarConfig = 0;
        if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig) == 0) {
            throw IOError(MODULE, "Failed to get TIFFTAG_PLANARCONFIG");
        }

        if (planarConfig == PLANARCONFIG_CONTIG) {
            builder.imageLayout(ImageLayout::INTERLEAVED);
        } else if (planarConfig == PLANARCONFIG_SEPARATE) {
            builder.imageLayout(ImageLayout::PLANAR);
        } else {
            throw IOError(MODULE, "Unsupported planar config " + std::to_string(planarConfig));
        }
    } else {
        throw IOError(MODULE, "Unsupported samples per pixel " + std::to_string(samplesPerPixel));
    }

    PixelRepresentation pixelRepresentation;
    if (sampleFormat == SAMPLEFORMAT_IEEEFP) {
        pixelRepresentation = PixelRepresentation::FLOAT;
    } else if (sampleFormat == SAMPLEFORMAT_UINT) {
        builder.pixelPrecision(bitsPerSample);

        if (bitsPerSample == 8) {
            pixelRepresentation = PixelRepresentation::UINT8;
        } else if (bitsPerSample == 16) {
            pixelRepresentation = PixelRepresentation::UINT16;
        } else {
            throw IOError(MODULE, "Unsupported bits per sample " + std::to_string(bitsPerSample));
        }
    } else {
        throw IOError(MODULE, "Unsupported sample format " + std::to_string(sampleFormat));
    }

    if (options.fileInfo.pixelPrecision) {
        builder.pixelPrecision(*options.fileInfo.pixelPrecision);
    }

    setDescriptor({builder.build(), pixelRepresentation});
}

Image8u TiffReader::read8u() {
    LOG_SCOPE_F(INFO, "Read TIFF (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint8_t>();
}

Image16u TiffReader::read16u() {
    LOG_SCOPE_F(INFO, "Read TIFF (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

Imagef TiffReader::readf() {
    LOG_SCOPE_F(INFO, "Read TIFF (float)");
    LOG_S(INFO) << "Path: " << path();

    return read<float>();
}

template <typename T>
Image<T> TiffReader::read() {
    validateType<T>();

    TIFF *tif = mTiff.get();
    const uint32_t nStrips = TIFFNumberOfStrips(tif);

    uint32_t rowsPerStrip = 0;
    if (TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip) == 0) {
        // TIFFTAG_ROWSPERSTRIP is optional if there is only one strip.
        if (nStrips > 1) {
            throw IOError(MODULE, "Failed to get TIFFTAG_ROWSPERSTRIP");
        }
    }

    Image<T> image(layoutDescriptor());
    T *pStrip = image.data();

    // Copy the TIFF image strips data to cxximg image.
    const int64_t rowStride = image.width() * image.numPlanes();
    for (tstrip_t strip = 0; strip < nStrips; ++strip) {
        TIFFReadEncodedStrip(tif, strip, pStrip + strip * (rowsPerStrip * rowStride), -1);
    }

    return image;
}

// Even if floating point values are stored as rationals in EXIF, libtiff can only return the already converted value,
// which forces us to convert it back to rational.
// See https://gitlab.com/libtiff/libtiff/-/issues/226 and https://gitlab.com/libtiff/libtiff/-/issues/531
static ExifMetadata::Rational doubleToRational(double x) {
    ExifMetadata::Rational r;
    TIFFDoubleToRational(x, &r.numerator, &r.denominator);
    return r;
}

std::optional<ExifMetadata> TiffReader::readExif() const {
    TIFF *tif = mTiff.get();

    uint64_t exifOffset = 0;
    if (TIFFGetField(tif, TIFFTAG_EXIFIFD, &exifOffset) == 0) {
        return std::nullopt;
    }

    ExifMetadata exif;

    char *imageDescription = nullptr;
    if (TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &imageDescription) != 0) {
        exif.imageDescription = imageDescription;
    }

    char *make = nullptr;
    if (TIFFGetField(tif, TIFFTAG_MAKE, &make) != 0) {
        exif.make = make;
    }

    char *model = nullptr;
    if (TIFFGetField(tif, TIFFTAG_MODEL, &model) != 0) {
        exif.model = model;
    }

    uint16_t orientation = 0;
    if (TIFFGetField(tif, TIFFTAG_ORIENTATION, &orientation) != 0) {
        // Orientation is bewteen 1 and 8.
        if (orientation > 0 && orientation < 9) {
            exif.orientation = orientation;
        }
    }

    char *software = nullptr;
    if (TIFFGetField(tif, TIFFTAG_SOFTWARE, &software) != 0) {
        exif.software = software;
    }

    TIFFReadEXIFDirectory(tif, exifOffset);

    float exposureTime = 0.0f;
    if (TIFFGetField(tif, EXIFTAG_EXPOSURETIME, &exposureTime) != 0) {
        exif.exposureTime = doubleToRational(exposureTime);
    }

    float fNumber = 0.0f;
    if (TIFFGetField(tif, EXIFTAG_FNUMBER, &fNumber) != 0) {
        exif.fNumber = doubleToRational(fNumber);
    }

    uint16_t count = 0;
    uint16_t *isoSpeedRatings = nullptr;
    if (TIFFGetField(tif, EXIFTAG_ISOSPEEDRATINGS, &count, &isoSpeedRatings) != 0) {
        exif.isoSpeedRatings = isoSpeedRatings[0];
    }

    char *dateTimeOriginal = nullptr;
    if (TIFFGetField(tif, EXIFTAG_DATETIMEORIGINAL, &dateTimeOriginal) != 0) {
        exif.dateTimeOriginal = dateTimeOriginal;
    }

    float focalLength = 0.0f;
    if (TIFFGetField(tif, EXIFTAG_FOCALLENGTH, &focalLength) != 0) {
        exif.focalLength = doubleToRational(focalLength);
    }

    uint16_t focalLengthIn35mmFilm = 0;
    if (TIFFGetField(tif, EXIFTAG_FOCALLENGTHIN35MMFILM, &focalLengthIn35mmFilm) != 0) {
        exif.focalLengthIn35mmFilm = focalLengthIn35mmFilm;
    }

    TIFFSetDirectory(tif, 0); // go back to main directory

    return exif;
}

static void populateIfd(TIFF *tif, const ExifMetadata &exif) {
    const uint64_t exifOffset = 0;
    TIFFSetField(tif, TIFFTAG_EXIFIFD, exifOffset); // reserve space, will be filled in populateExif

    if (exif.imageDescription) {
        TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, (*exif.imageDescription).c_str());
    }
    if (exif.make) {
        TIFFSetField(tif, TIFFTAG_MAKE, (*exif.make).c_str());
    }
    if (exif.model) {
        TIFFSetField(tif, TIFFTAG_MODEL, (*exif.model).c_str());
    }
    if (exif.orientation) {
        TIFFSetField(tif, TIFFTAG_ORIENTATION, *exif.orientation);
    }
    if (exif.software) {
        TIFFSetField(tif, TIFFTAG_SOFTWARE, (*exif.software).c_str());
    }
}

static void populateExif(TIFF *tif, const ExifMetadata &exif) {
    const char exifVersion[] = {'0', '2', '3', '1'};
    TIFFSetField(tif, EXIFTAG_EXIFVERSION, exifVersion);

    if (exif.exposureTime) {
        TIFFSetField(tif, EXIFTAG_EXPOSURETIME, (*exif.exposureTime).asFloat());
    }
    if (exif.fNumber) {
        TIFFSetField(tif, EXIFTAG_FNUMBER, (*exif.fNumber).asFloat());
    }
    if (exif.isoSpeedRatings) {
        TIFFSetField(tif, EXIFTAG_ISOSPEEDRATINGS, 1, &(*exif.isoSpeedRatings));
    }
    if (exif.dateTimeOriginal) {
        TIFFSetField(tif, EXIFTAG_DATETIMEORIGINAL, (*exif.dateTimeOriginal).c_str());
    }
    if (exif.focalLength) {
        TIFFSetField(tif, EXIFTAG_FOCALLENGTH, (*exif.focalLength).asFloat());
    }
    if (exif.focalLengthIn35mmFilm) {
        TIFFSetField(tif, EXIFTAG_FOCALLENGTHIN35MMFILM, *exif.focalLengthIn35mmFilm);
    }
}

void TiffWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write TIFF (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint8_t>(image);
}

void TiffWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write TIFF (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint16_t>(image);
}

void TiffWriter::write(const Imagef &image) const {
    LOG_SCOPE_F(INFO, "Write TIFF (float)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<float>(image);
}

template <typename T>
void TiffWriter::writeImpl(const Image<T> &image) const {
    if (image.imageLayout() == ImageLayout::PLANAR && image.numPlanes() > 1) {
        // Planar to interleaved conversion
        return writeImpl<T>(image::convertLayout(image, ImageLayout::INTERLEAVED));
    }

    std::unique_ptr<TIFF, TiffDeleter> tiffPtr(TIFFOpen(path().c_str(), "w"));
    if (!tiffPtr) {
        throw IOError(MODULE, "Cannot open output file for writing");
    }
    TIFF *tif = tiffPtr.get();

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, image.width());
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, image.height());
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, image.numPlanes());
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, sizeof(T) * 8);

    if constexpr (std::is_floating_point_v<T>) {
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    }

    if (image::isBayerPixelType(image.pixelType())) {
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_CFA);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        const int16_t cfaPatternDim[] = {2, 2};
        TIFFSetField(tif, TIFFTAG_CFAREPEATPATTERNDIM, cfaPatternDim);
    }

    switch (image.pixelType()) {
        case PixelType::GRAYSCALE:
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            break;
        case PixelType::BAYER_BGGR:
        case PixelType::QUADBAYER_BGGR:
            TIFFSetField(tif, TIFFTAG_CFAPATTERN, 4, "\02\01\01\00");
            break;
        case PixelType::BAYER_GBRG:
        case PixelType::QUADBAYER_GBRG:
            TIFFSetField(tif, TIFFTAG_CFAPATTERN, 4, "\01\02\00\01");
            break;
        case PixelType::BAYER_GRBG:
        case PixelType::QUADBAYER_GRBG:
            TIFFSetField(tif, TIFFTAG_CFAPATTERN, 4, "\01\00\02\01");
            break;
        case PixelType::BAYER_RGGB:
        case PixelType::QUADBAYER_RGGB:
            TIFFSetField(tif, TIFFTAG_CFAPATTERN, 4, "\00\01\01\02");
            break;
        case PixelType::RGB:
            // TIFF RGB images should always have a contiguous (RGBRGBRGB ...) not planar (RRR..RGGG..GBBB..B) format.
            // While the header allows you to specify planar, it is not well supported.
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            break;
        default:
            throw IOError(MODULE, "Unsupported pixel type "s + toString(image.pixelType()));
    }

    const uint32_t rowsPerStrip = TIFFDefaultStripSize(tif, -1);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);

    if (options().tiffCompression == TiffCompression::DEFLATE) {
        LOG_S(INFO) << "Compression: zip";
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
        TIFFSetField(tif, TIFFTAG_ZIPQUALITY, 3);

        if constexpr (std::is_floating_point_v<T>) {
            TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_FLOATINGPOINT);
        } else {
            TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        }
    } else {
        LOG_S(INFO) << "Compression: none";
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    }

    const auto &metadata = options().metadata;
    if (metadata) {
        populateIfd(tif, metadata->exifMetadata);
    }

    // Write image data.
    const T *pStrip = image.data();
    const int64_t rowStride = image.width() * image.numPlanes();
    tmsize_t stripSize = TIFFStripSize(tif);

    tstrip_t strip = 0;
    for (int row = 0; row < image.height(); row += rowsPerStrip) {
        if (row + static_cast<int>(rowsPerStrip) > image.height()) {
            stripSize = TIFFVStripSize(tif, image.height() - row);
        }
        if (TIFFWriteEncodedStrip(tif, strip, const_cast<T *>(pStrip + row * rowStride), stripSize) < 0) {
            throw IOError(MODULE, "An error occured while writing");
        }
        ++strip;
    }

    // Write IFD
    TIFFWriteDirectory(tif);

    if (metadata) {
        TIFFCreateEXIFDirectory(tif);
        populateExif(tif, metadata->exifMetadata);

        uint64_t exifOffset = 0;
        TIFFWriteCustomDirectory(tif, &exifOffset);

        TIFFSetDirectory(tif, 0); // go back to main directory
        TIFFSetField(tif, TIFFTAG_EXIFIFD, exifOffset);
        TIFFWriteDirectory(tif);
    }
}

void TiffWriter::writeExif(const ExifMetadata &exif) const {
    std::unique_ptr<TIFF, TiffDeleter> tiffPtr(TIFFOpen(path().c_str(), "r+"));
    if (!tiffPtr) {
        throw IOError(MODULE, "Cannot open output file for read/write");
    }
    TIFF *tif = tiffPtr.get();

    // Write IFD
    populateIfd(tif, exif);
    TIFFRewriteDirectory(tif);

    TIFFCreateEXIFDirectory(tif);
    populateExif(tif, exif);

    uint64_t exifOffset = 0;
    TIFFWriteCustomDirectory(tif, &exifOffset);

    TIFFSetDirectory(tif, 0); // go back to main directory
    TIFFSetField(tif, TIFFTAG_EXIFIFD, exifOffset);
    TIFFWriteDirectory(tif);
}

} // namespace cxximg
