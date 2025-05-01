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

#include <cxximg/model/ExifMetadata.h>

#include <libexif/exif-data.h>

namespace cxximg {

namespace detail {

inline std::optional<ExifMetadata> readExif(const unsigned char *buffer, unsigned size) {
    ExifData *data = exif_data_new_from_data(buffer, size);
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
                    case EXIF_TAG_LENS_MAKE:
                        exif->lensMake = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    case EXIF_TAG_LENS_MODEL:
                        exif->lensModel = std::string(reinterpret_cast<const char *>(entry->data), entry->size);
                        break;
                    default:
                        break;
                }
            },
            &exif);

    exif_data_unref(data);

    return exif;
}

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

inline void populateExif(ExifMem *mem, ExifData *data, ExifMetadata exif) {
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
    if (exif.lensMake) {
        entry = addExifEntry(ifdExif, EXIF_TAG_LENS_MAKE);
        exifSetString(mem, entry, *exif.lensMake);
    }
    if (exif.lensModel) {
        entry = addExifEntry(ifdExif, EXIF_TAG_LENS_MODEL);
        exifSetString(mem, entry, *exif.lensModel);
    }
}

} // namespace detail

} // namespace cxximg
