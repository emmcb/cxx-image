#include "RawlerIO.h"

#include "cxximg/math/ColorSpace.h"

#ifdef HAVE_LENSFUN
#include "cxximg/io/LensfunDatabase.h"
#endif

#include <rawler_ffi.h>
#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "RAWLER";

RawlerReader::RawlerReader(const std::string& path, std::istream* stream, const Options& options)
    : ImageReader(path, stream, options) {
}

RawlerReader::~RawlerReader() {
    if (mRawImage) {
        rawler::free_image(mRawImage);
    }
}

void RawlerReader::initialize() {
    // Get stream size
    mStream->seekg(0, std::istream::end);
    const int64_t fileSize = mStream->tellg();
    mStream->seekg(0);

    // Read stream content
    std::vector<uint8_t> data(fileSize);
    mStream->read(reinterpret_cast<char*>(data.data()), data.size());

    // Decode raw data
    char* errorMsg = nullptr;
    mRawImage = rawler::decode_buffer(data.data(), data.size(), &errorMsg);
    if (!mRawImage) {
        throw IOError(MODULE, errorMsg);
    }

    if (mRawImage->cpp != 1) {
        throw IOError(MODULE, "Unsupported number of channels: " + std::to_string(mRawImage->cpp));
    }

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(mRawImage->active_area[2], mRawImage->active_area[3]);
    if (mRawImage->cfa == "RGGB"s) {
        builder.pixelType(PixelType::BAYER_RGGB);
    } else if (mRawImage->cfa == "GRBG"s) {
        builder.pixelType(PixelType::BAYER_GRBG);
    } else if (mRawImage->cfa == "GBRG"s) {
        builder.pixelType(PixelType::BAYER_GBRG);
    } else if (mRawImage->cfa == "BGGR"s) {
        builder.pixelType(PixelType::BAYER_BGGR);
    } else if ((mRawImage->cfa == "RBGBRGGGRGGBGGBGGRBRGRBGGGBGGRGGRGGB"s && mRawImage->active_area[0] == 0 &&
                mRawImage->active_area[1] == 0) ||
               (mRawImage->cfa == "GGRGGBGGBGGRBRGRBGGGBGGRGGRGGBRBGBRG"s && mRawImage->active_area[0] == 0 &&
                mRawImage->active_area[1] == 5)) {
        builder.pixelType(PixelType::X_TRANS);
    } else {
        throw IOError(MODULE, "Unsupported CFA pattern: "s + mRawImage->cfa);
    }

    PixelRepresentation pixelRepresentation = [&]() {
        if (mRawImage->data_type == rawler::DataType::Float) {
            return PixelRepresentation::FLOAT;
        }

        builder.pixelPrecision(std::ceil(std::log2(mRawImage->white_levels[0])));
        return PixelRepresentation::UINT16;
    }();

    mDescriptor = {builder.build(), pixelRepresentation};
}

Image16u RawlerReader::read16u() {
    LOG_SCOPE_F(INFO, "Read RAW (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

Imagef RawlerReader::readf() {
    LOG_SCOPE_F(INFO, "Read RAW (float)");
    LOG_S(INFO) << "Path: " << path();

    return read<float>();
}

template <typename T>
Image<T> RawlerReader::read() {
    validateType<T>();

    LayoutDescriptor srcDescriptor = LayoutDescriptor::Builder(layoutDescriptor())
                                             .width(mRawImage->width)
                                             .height(mRawImage->height)
                                             .build();

    if (static_cast<int64_t>(mRawImage->data_len) != srcDescriptor.requiredBufferSize()) {
        throw IOError(MODULE,
                      "Data length does not match expected buffer size (expected " +
                              std::to_string(srcDescriptor.requiredBufferSize()) + ", got " +
                              std::to_string(mRawImage->data_len) + ")");
    }

    Image<T> image(layoutDescriptor());
    ImageView16u srcImage(srcDescriptor, reinterpret_cast<uint16_t*>(const_cast<void*>(mRawImage->data_ptr)));
    Rect crop{.x = static_cast<int>(mRawImage->active_area[0]),
              .y = static_cast<int>(mRawImage->active_area[1]),
              .width = static_cast<int>(mRawImage->active_area[2]),
              .height = static_cast<int>(mRawImage->active_area[3])};

    // Copy raw data to the image
    image = srcImage[crop];

    return image;
}

std::optional<ExifMetadata> RawlerReader::readExif() const {
    ExifMetadata exif;
    exif.make = mRawImage->make;
    exif.model = mRawImage->model;

    if (mRawImage->exif.orientation > 0) {
        exif.orientation = mRawImage->exif.orientation;
    }
    if (mRawImage->exif.exposure_time[1] > 0) {
        exif.exposureTime = {mRawImage->exif.exposure_time[0], mRawImage->exif.exposure_time[1]};
    }
    if (mRawImage->exif.fnumber[1] > 0) {
        exif.fNumber = {mRawImage->exif.fnumber[0], mRawImage->exif.fnumber[1]};
    }
    if (mRawImage->exif.iso_speed_ratings > 0) {
        exif.isoSpeedRatings = mRawImage->exif.iso_speed_ratings;
    }
    if (mRawImage->exif.date_time_original[0] != 0) {
        exif.dateTimeOriginal = mRawImage->exif.date_time_original;
    }
    if (mRawImage->exif.brightness_value[1] > 0) {
        exif.brightnessValue = {mRawImage->exif.brightness_value[0], mRawImage->exif.brightness_value[1]};
    }
    if (mRawImage->exif.exposure_bias[1] > 0) {
        exif.exposureBiasValue = {mRawImage->exif.exposure_bias[0], mRawImage->exif.exposure_bias[1]};
    }
    if (mRawImage->exif.focal_length[1] > 0) {
        exif.focalLength = {mRawImage->exif.focal_length[0], mRawImage->exif.focal_length[1]};
    }
    if (mRawImage->exif.lens_make[0] != 0) {
        exif.lensMake = mRawImage->exif.lens_make;
    }
    if (mRawImage->exif.lens_model[0] != 0) {
        exif.lensModel = mRawImage->exif.lens_model;
    }

    return exif;
}

namespace {

#ifdef HAVE_LENSFUN

void readLensfunMetadata(ImageMetadata& metadata) {
    const ExifMetadata& exif = metadata.exifMetadata;

    if (!exif.make || !exif.model) {
        LOG_S(INFO) << "Skipping lens correction: no camera make/model in metadata";
        return;
    }

    if (!exif.focalLength) {
        LOG_S(INFO) << "Skipping lens correction: no focal length in metadata";
        return;
    }

    lfDatabase* db = io::lensfunDatabase();
    if (!db) {
        LOG_S(WARNING) << "Skipping lens correction: lensfun database not available";
        return;
    }

    // Find camera
    const lfCamera** cameras = db->FindCameras(exif.make->c_str(), exif.model->c_str());
    if (!cameras) {
        LOG_S(INFO) << "Skipping lens correction: camera not found in lensfun database (" << *exif.make << " "
                    << *exif.model << ")";
        return;
    }
    if (cameras[1] != nullptr) {
        LOG_S(INFO) << "Skipping lens correction: multiple cameras found in lensfun database";
        lf_free(cameras);
        return;
    }
    const lfCamera* camera = cameras[0];
    const float cropFactor = camera->CropFactor;
    LOG_S(INFO) << "Lensfun camera: " << lf_mlstr_get(camera->Maker) << " " << lf_mlstr_get(camera->Model) << " (crop "
                << cropFactor << ")";

    // Find lens
    const char* lensMaker = exif.lensMake ? exif.lensMake->c_str() : nullptr;
    const char* lensModel = exif.lensModel ? exif.lensModel->c_str() : nullptr;
    const lfLens** lenses = db->FindLenses(camera, lensMaker, lensModel);
    if (!lenses) {
        LOG_S(INFO) << "Skipping lens correction: lens not found in lensfun database";
        lf_free(cameras);
        return;
    }
    if (lenses[1] != nullptr) {
        LOG_S(INFO) << "Skipping lens correction: multiple lenses found in lensfun database";
        lf_free(cameras);
        lf_free(lenses);
        return;
    }
    const lfLens* lens = lenses[0];
    LOG_S(INFO) << "Lensfun lens: " << lf_mlstr_get(lens->Maker) << " " << lf_mlstr_get(lens->Model);

    const int width = 17;
    const int height = 13;
    const float focal = exif.focalLength->asFloat();
    const float aperture = exif.fNumber ? exif.fNumber->asFloat() : 8.0f;

    // Create modifier for distortion and TCA correction
    lfModifier modifier(lens, cropFactor, width, height);
    modifier.Initialize(lens,
                        LF_PF_F32,
                        focal,
                        aperture,
                        1000.0f, // focus distance (approximation)
                        0.0f,    // auto scale
                        lens->Type,
                        LF_MODIFY_VIGNETTING,
                        false);

    DynamicMatrix gainMap(height, width, 1.0f);
    modifier.ApplyColorModification(gainMap.data(), 0, 0, width, height, LF_CR_1(GREEN), width * sizeof(float));

    metadata.calibrationData.vignetting = std::move(gainMap);

    lf_free(cameras);
    lf_free(lenses);
}

#endif // HAVE_LENSFUN

} // namespace

std::optional<ImageMetadata> RawlerReader::readMetadata(const std::optional<ImageMetadata>& baseMetadata) const {
    ImageMetadata metadata = ImageReader::readMetadata(baseMetadata).value();

    metadata.cameraControls.whiteBalance = {mRawImage->wb_coeffs[0], mRawImage->wb_coeffs[2]};

    metadata.calibrationData.blackLevel = static_cast<int>(mRawImage->black_levels[0]);
    metadata.calibrationData.whiteLevel = static_cast<int>(mRawImage->white_levels[0]);

    // DNG color matrix is XYZ to Camera matrix, thus we need to compute the Camera to SRGB matrix.
    const Matrix3 colorMatrix(mRawImage->color_matrix);

    const Pixel3f cameraNeutral{1.0f / mRawImage->wb_coeffs[0], 1.0f, 1.0f / mRawImage->wb_coeffs[2]};
    const Pixel3f cameraWhite = [&]() {
        const auto xyz = colorMatrix.inverse() * cameraNeutral;
        return xyz / xyz[1];
    }();

    const Matrix3 adaptedMatrix = colorMatrix *
                                  colorspace::linearBradfordAdaptation(colorspace::D50_WHITE_XYZ, cameraWhite);

    const Matrix3 forwardMatrix = colorspace::transformationMatrix(RgbColorSpace::XYZ_D50, RgbColorSpace::SRGB) *
                                  adaptedMatrix.inverse() * Matrix3::diag(cameraNeutral);

    const float invScale = (adaptedMatrix * colorspace::D50_WHITE_XYZ).maximum();
    metadata.calibrationData.colorMatrix = invScale * forwardMatrix;

#ifdef HAVE_LENSFUN
    readLensfunMetadata(metadata);
#endif

    return metadata;
}

} // namespace cxximg
