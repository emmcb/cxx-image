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

#include "JpegIO.h"

#include <jpeglib.h>
#include <loguru.hpp>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif

#include <csetjmp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "JPEG";

namespace {

struct JpegErrorMgr {
    jpeg_error_mgr pub;    ///< "public" fields
    jmp_buf setjmp_buffer; ///< for return to caller
};

struct JpegReadStream {
    jpeg_source_mgr pub;  ///< "public" fields
    std::istream *stream; ///< source stream
    uint8_t buffer[4096]; ///< start of buffer
    bool start_of_file;   ///< have we gotten any data yet?
};

struct JpegWriteStream {
    jpeg_destination_mgr pub; ///< "public" fields
    std::ostream *stream;     ///< destination stream
    uint8_t buffer[4096];     ///< start of buffer
};

void errorExit(j_common_ptr info) {
    // info->err really points to a CustomErrorMgr struct, so coerce pointer
    auto *jerr = reinterpret_cast<JpegErrorMgr *>(info->err);

    // Return control to the setjmp point
    longjmp(jerr->setjmp_buffer, 1); // NOLINT(cert-err52-cpp)
}

void outputMessage(j_common_ptr info) {
    char buffer[JMSG_LENGTH_MAX];

    // Create the message
    (*info->err->format_message)(info, buffer);

    LOG_S(WARNING) << buffer;
}

void initSource(j_decompress_ptr dinfo) {
    auto *src = reinterpret_cast<JpegReadStream *>(dinfo->src);
    src->start_of_file = true;
}

boolean fillInputBuffer(j_decompress_ptr dinfo) {
    auto *src = reinterpret_cast<JpegReadStream *>(dinfo->src);
    src->stream->read(reinterpret_cast<char *>(src->buffer), sizeof(src->buffer));

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = src->stream->gcount();

    if (src->pub.bytes_in_buffer <= 0) {
        // Insert a fake EOI marker
        src->buffer[0] = 0xFF;
        src->buffer[1] = JPEG_EOI;
        src->pub.next_input_byte = src->buffer;
        src->pub.bytes_in_buffer = 2;
    }

    return TRUE;
}

void skipInputData(j_decompress_ptr dinfo, long numBytes) { // NOLINT(google-runtime-int)
    if (numBytes <= 0) {
        return;
    }

    auto *src = reinterpret_cast<JpegReadStream *>(dinfo->src);

    if (static_cast<uint64_t>(numBytes) <= src->pub.bytes_in_buffer) {
        src->pub.next_input_byte += numBytes;
        src->pub.bytes_in_buffer -= numBytes;
    } else {
        src->stream->seekg(numBytes - src->pub.bytes_in_buffer, std::istream::cur);
        src->pub.next_input_byte = nullptr;
        src->pub.bytes_in_buffer = 0;
    }
}

void termSource([[maybe_unused]] j_decompress_ptr dinfo) {
    // nothing to do
}

void initDestination(j_compress_ptr cinfo) {
    auto *dest = reinterpret_cast<JpegWriteStream *>(cinfo->dest);
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = sizeof(dest->buffer);
}

boolean emptyOutputBuffer(j_compress_ptr cinfo) {
    auto *dest = reinterpret_cast<JpegWriteStream *>(cinfo->dest);

    dest->stream->write(reinterpret_cast<const char *>(dest->buffer), sizeof(dest->buffer));

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = sizeof(dest->buffer);

    return TRUE;
}

void termDestination(j_compress_ptr cinfo) {
    auto *dest = reinterpret_cast<JpegWriteStream *>(cinfo->dest);
    int64_t datacount = sizeof(dest->buffer) - dest->pub.free_in_buffer;

    // Write any data remaining in the buffer
    if (datacount > 0) {
        dest->stream->write(reinterpret_cast<const char *>(dest->buffer), datacount);
    }
}

} // namespace

static void setupJpegSourceStream(j_decompress_ptr dinfo, std::istream *stream) {
    auto *src = static_cast<JpegReadStream *>(
            (*dinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(dinfo), JPOOL_PERMANENT, sizeof(JpegReadStream)));
    dinfo->src = &src->pub;
    src->pub.init_source = initSource;
    src->pub.fill_input_buffer = fillInputBuffer;
    src->pub.skip_input_data = skipInputData;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = termSource;
    src->pub.bytes_in_buffer = 0;       /* forces fill_input_buffer on first read */
    src->pub.next_input_byte = nullptr; /* until buffer loaded */
    src->stream = stream;
}

static void setupJpegDestinationStream(j_compress_ptr cinfo, std::ostream *stream) {
    auto *dest = static_cast<JpegWriteStream *>((*cinfo->mem->alloc_small)(
            reinterpret_cast<j_common_ptr>(cinfo), JPOOL_PERMANENT, sizeof(JpegWriteStream)));
    cinfo->dest = &dest->pub;
    dest->pub.init_destination = initDestination;
    dest->pub.empty_output_buffer = emptyOutputBuffer;
    dest->pub.term_destination = termDestination;
    dest->stream = stream;
}

void JpegDecompressDeleter::operator()(jpeg_decompress_struct *dinfo) const {
    jpeg_destroy_decompress(dinfo);

    auto *jerr = reinterpret_cast<JpegErrorMgr *>(dinfo->err);
    delete jerr;

    delete dinfo;
}

void JpegReader::initialize() {
    mInfo.reset(new jpeg_decompress_struct());
    jpeg_decompress_struct *dinfo = mInfo.get();

    auto *jerr = new JpegErrorMgr();
    dinfo->err = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = errorExit;
    jerr->pub.output_message = outputMessage;

    if (setjmp(jerr->setjmp_buffer)) { // NOLINT(cert-err52-cpp)
        throw IOError(MODULE, "Reading failed");
    }

    jpeg_create_decompress(dinfo);

    setupJpegSourceStream(dinfo, mStream);

    jpeg_save_markers(dinfo, JPEG_APP0 + 1, 0xFFFF);
    jpeg_read_header(dinfo, TRUE);

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(dinfo->image_width, dinfo->image_height)
                                                .pixelPrecision(8);

    if (dinfo->num_components == 1) {
        builder.pixelType(PixelType::GRAYSCALE);
    } else if (dinfo->num_components == 3) {
        builder.imageLayout(ImageLayout::INTERLEAVED);
        if (options().jpegDecodingMode == ImageReader::JpegDecodingMode::YUV) {
            builder.pixelType(PixelType::YUV);
            dinfo->out_color_space = JCS_YCbCr;
        } else {
            builder.pixelType(PixelType::RGB);
        }
    } else {
        throw IOError(MODULE, "Unsupported number of components: " + std::to_string(dinfo->num_components));
    }

    mDescriptor = {builder.build(), PixelRepresentation::UINT8};
}

Image8u JpegReader::read8u() {
    LOG_SCOPE_F(INFO, "Read JPEG");
    LOG_S(INFO) << "Path: " << path();

    Image8u image(layoutDescriptor());

    jpeg_decompress_struct *dinfo = mInfo.get();

    auto *jerr = reinterpret_cast<JpegErrorMgr *>(dinfo->err);
    if (setjmp(jerr->setjmp_buffer)) { // NOLINT(cert-err52-cpp)
        throw IOError(MODULE, "Reading failed");
    }

    jpeg_start_decompress(dinfo);

    for (int y = 0; y < image.height(); ++y) {
        auto *row = image.buffer(0, y);
        jpeg_read_scanlines(dinfo, &row, 1);
    }

    jpeg_finish_decompress(dinfo);

    return image;
}

#ifdef HAVE_EXIF
std::optional<ExifMetadata> JpegReader::readExif() const {
    jpeg_decompress_struct *dinfo = mInfo.get();

    if (!dinfo->marker_list) {
        return std::nullopt;
    }

    ExifData *data = exif_data_new_from_data(dinfo->marker_list->data, dinfo->marker_list->data_length);
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
                ExifSRational srational;

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
                    case EXIF_TAG_BRIGHTNESS_VALUE:
                        srational = exif_get_srational(entry->data, byteOrder);
                        exif->brightnessValue = {srational.numerator, srational.denominator};
                        break;
                    case EXIF_TAG_EXPOSURE_BIAS_VALUE:
                        srational = exif_get_srational(entry->data, byteOrder);
                        exif->exposureBiasValue = {srational.numerator, srational.denominator};
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

#ifdef HAVE_EXIF
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
    if (exif.brightnessValue) {
        entry = addExifEntry(ifdExif, EXIF_TAG_BRIGHTNESS_VALUE);
        exif_set_srational(
                entry->data, FILE_BYTE_ORDER, {exif.brightnessValue->numerator, exif.brightnessValue->denominator});
    }
    if (exif.exposureBiasValue) {
        entry = addExifEntry(ifdExif, EXIF_TAG_EXPOSURE_BIAS_VALUE);
        exif_set_srational(
                entry->data, FILE_BYTE_ORDER, {exif.exposureBiasValue->numerator, exif.exposureBiasValue->denominator});
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
    if (image.imageLayout() == ImageLayout::PLANAR && image.numPlanes() > 1) {
        // Planar to interleaved conversion
        return write(image::convertLayout(image, ImageLayout::INTERLEAVED));
    }
    if (image.imageLayout() == ImageLayout::NV12) {
        // NV12 to YUV_420 conversion
        return write(image::convertLayout(image, ImageLayout::YUV_420));
    }

    LOG_SCOPE_F(INFO, "Write JPEG");
    LOG_S(INFO) << "Path: " << path();

    jpeg_compress_struct cinfo{};

    JpegErrorMgr jerr{};
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = errorExit;
    jerr.pub.output_message = outputMessage;

    if (setjmp(jerr.setjmp_buffer)) { // NOLINT(cert-err52-cpp)
        jpeg_destroy_compress(&cinfo);
        throw IOError(MODULE, "Writing failed");
    }

    jpeg_create_compress(&cinfo);

    setupJpegDestinationStream(&cinfo, mStream);

    cinfo.image_width = image.width();
    cinfo.image_height = image.height();
    cinfo.input_components = image.numPlanes();

    switch (image.pixelType()) {
        case PixelType::GRAYSCALE:
            cinfo.in_color_space = JCS_GRAYSCALE;
            break;
        case PixelType::RGB:
            cinfo.in_color_space = JCS_RGB;
            break;
        case PixelType::YUV:
            cinfo.in_color_space = JCS_YCbCr;
            break;
        default:
            throw IOError(MODULE, "Unsupported pixel type: "s + toString(image.pixelType()));
    }

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, options().jpegQuality, FALSE);

    if (image.imageLayout() == ImageLayout::YUV_420) {
        cinfo.raw_data_in = TRUE;
    }

    jpeg_start_compress(&cinfo, TRUE);

#ifdef HAVE_EXIF
    // Write EXIF
    const auto &metadata = options().metadata;
    if (metadata) {
        ExifMem *mem = exif_mem_new_default();
        ExifData *data = exif_data_new();

        populateExif(mem, data, metadata->exifMetadata);

        uint8_t *exifBuffer = nullptr;
        uint32_t exifLength = 0;
        exif_data_save_data(data, &exifBuffer, &exifLength);

        jpeg_write_marker(&cinfo, JPEG_APP0 + 1, exifBuffer, exifLength);

        free(exifBuffer); // NOLINT(cppcoreguidelines-no-malloc)
        exif_mem_unref(mem);
        exif_data_unref(data);
    }
#endif

    if (image.imageLayout() == ImageLayout::YUV_420) {
        uint8_t *yRows[16];
        uint8_t *uRows[8];
        uint8_t *vRows[8];

        uint8_t **rows[3] = {yRows, uRows, vRows};
        for (int y = 0; y < image.height(); y += 16) {
            // Jpeg library ignores the rows whose indices are greater than height
            for (int i = 0; i < 16; i++) {
                yRows[i] = image.buffer(0, y + i);
            }
            for (int i = 0; i < 8; i++) {
                uRows[i] = image.buffer(1, (y >> 1) + i);
                vRows[i] = image.buffer(2, (y >> 1) + i);
            }

            jpeg_write_raw_data(&cinfo, rows, 16);
        }
    } else {
        for (int y = 0; y < image.height(); ++y) {
            auto *row = image.buffer(0, y);
            jpeg_write_scanlines(&cinfo, &row, 1);
        }
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

#ifdef HAVE_EXIF
void JpegWriter::writeExif(const ExifMetadata &exif) const {
    std::ifstream ifs(path(), std::ios::binary);
    if (!ifs) {
        throw IOError(MODULE, "Cannot open file for reading: " + path());
    }

    jpeg_decompress_struct dinfo{};
    jpeg_compress_struct cinfo{};

    // Initialize the JPEG decompression object
    JpegErrorMgr jderr{};
    dinfo.err = jpeg_std_error(&jderr.pub);
    jderr.pub.error_exit = errorExit;
    jderr.pub.output_message = outputMessage;

    if (setjmp(jderr.setjmp_buffer)) { // NOLINT(cert-err52-cpp)
        jpeg_destroy_compress(&cinfo);
        jpeg_destroy_decompress(&dinfo);
        throw IOError(MODULE, "Reading failed");
    }

    jpeg_create_decompress(&dinfo);
    setupJpegSourceStream(&dinfo, &ifs);

    // Initialize the JPEG compression object
    JpegErrorMgr jcerr{};
    cinfo.err = jpeg_std_error(&jcerr.pub);
    jcerr.pub.error_exit = errorExit;
    jcerr.pub.output_message = outputMessage;

    if (setjmp(jcerr.setjmp_buffer)) { // NOLINT(cert-err52-cpp)
        jpeg_destroy_compress(&cinfo);
        jpeg_destroy_decompress(&dinfo);
        throw IOError(MODULE, "Writing failed");
    }

    jpeg_create_compress(&cinfo);

    // Read source DCT coefficients
    jpeg_read_header(&dinfo, TRUE);
    jvirt_barray_ptr *coefficients = jpeg_read_coefficients(&dinfo);

    // Initialize destination compression parameters from source values
    jpeg_copy_critical_parameters(&dinfo, &cinfo);

    // Close input file and open output file
    ifs.close();
    std::ofstream ofs(path(), std::ios::binary);
    setupJpegDestinationStream(&cinfo, &ofs);

    // Write destination DCT coefficients
    jpeg_write_coefficients(&cinfo, coefficients);

    // Write EXIF
    ExifMem *mem = exif_mem_new_default();
    ExifData *data = exif_data_new();

    populateExif(mem, data, exif);

    uint8_t *exifBuffer = nullptr;
    uint32_t exifLength = 0;
    exif_data_save_data(data, &exifBuffer, &exifLength);

    jpeg_write_marker(&cinfo, JPEG_APP0 + 1, exifBuffer, exifLength);

    free(exifBuffer); // NOLINT(cppcoreguidelines-no-malloc)
    exif_mem_unref(mem);
    exif_data_unref(data);

    // Finish compression and release memory
    jpeg_finish_compress(&cinfo);
    jpeg_finish_decompress(&dinfo);

    jpeg_destroy_compress(&cinfo);
    jpeg_destroy_decompress(&dinfo);
}
#endif

} // namespace cxximg
