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

#include "DngIO.h"

#include "cxximg/math/ColorSpace.h"

#include <dng_color_space.h>
#include <dng_color_spec.h>
#include <dng_file_stream.h>
#include <dng_gain_map.h>
#include <dng_host.h>
#include <dng_image_writer.h>
#include <dng_info.h>
#include <loguru.hpp>

#include <unordered_map>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "DNG";

namespace {

class DngReadStream final : public dng_stream {
public:
    explicit DngReadStream(std::istream *stream)
        : dng_stream(static_cast<dng_abort_sniffer *>(nullptr), kDefaultBufferSize, 0), mStream(stream) {}

protected:
    uint64 DoGetLength() override {
        mStream->seekg(0, std::istream::end);
        return mStream->tellg();
    }

    void DoRead(void *data, uint32 count, uint64 offset) override {
        mStream->seekg(offset);
        mStream->read(reinterpret_cast<char *>(data), count);

        if (mStream->fail()) {
            ThrowReadFile();
        }
    }

private:
    std::istream *mStream;
};

class DngWriteStream final : public dng_stream {
public:
    explicit DngWriteStream(std::ostream *stream)
        : dng_stream(static_cast<dng_abort_sniffer *>(nullptr), kDefaultBufferSize, 0), mStream(stream) {}

protected:
    uint64 DoGetLength() override { return 0; }

    void DoWrite(const void *data, uint32 count, uint64 offset) override {
        mStream->seekp(offset);
        mStream->write(reinterpret_cast<const char *>(data), count);

        if (mStream->fail()) {
            ThrowWriteFile();
        }
    }

private:
    std::ostream *mStream;
};

} // namespace

DngReader::DngReader(const std::string &path, std::istream *stream, const Options &options)
    : ImageReader(path, stream, options) {
}

DngReader::~DngReader() = default;

static PixelType cfaPatternToPixelType(const dng_ifd *ifd) {
    const uint8_t colorRed = ifd->fCFAPlaneColor[0];
    const uint8_t colorGreen = ifd->fCFAPlaneColor[1];
    const uint8_t colorBlue = ifd->fCFAPlaneColor[2];

    if (ifd->fCFAPattern[0][0] == colorRed && ifd->fCFAPattern[0][1] == colorGreen) {
        return PixelType::BAYER_RGGB;
    }

    if (ifd->fCFAPattern[0][0] == colorGreen && ifd->fCFAPattern[0][1] == colorRed) {
        return PixelType::BAYER_GRBG;
    }

    if (ifd->fCFAPattern[0][0] == colorGreen && ifd->fCFAPattern[0][1] == colorBlue) {
        return PixelType::BAYER_GBRG;
    }

    if (ifd->fCFAPattern[0][0] == colorBlue && ifd->fCFAPattern[0][1] == colorGreen) {
        return PixelType::BAYER_BGGR;
    }

    throw IOError(MODULE,
                  "Unsupported CFA pattern " + std::to_string(ifd->fCFAPattern[0][0]) + " " +
                          std::to_string(ifd->fCFAPattern[0][1]));
}

void DngReader::readHeader() {
    mStream = std::make_unique<DngReadStream>(ImageReader::mStream);
    mHost = std::make_unique<dng_host>();
    mInfo = std::make_unique<dng_info>();
    mNegative.reset(mHost->Make_dng_negative());

    try {
        // Parse top-level info structure from stream.
        mInfo->Parse(*mHost, *mStream);
        mInfo->PostParse(*mHost);

        if (!mInfo->IsValidDNG()) {
            throw IOError(MODULE, "Invalid DNG image");
        }

        // Parse image negative.
        mNegative->Parse(*mHost, *mStream, *mInfo);
        mNegative->PostParse(*mHost, *mStream, *mInfo);
    } catch (const dng_exception &except) {
        throw IOError(MODULE, "Reading failed with error code " + std::to_string(except.ErrorCode()));
    }

    const dng_ifd *ifd = mInfo->fIFD[mInfo->fMainIndex];
    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(ifd->fActiveArea.W(), ifd->fActiveArea.H());

    if (ifd->fSamplesPerPixel == 1) {
        if (ifd->fPhotometricInterpretation != piCFA) {
            throw IOError(MODULE, "Unsupported photo metric " + std::to_string(ifd->fPhotometricInterpretation));
        }
        builder.pixelType(cfaPatternToPixelType(ifd));
    } else if (ifd->fSamplesPerPixel == 3) {
        if (ifd->fPhotometricInterpretation != piLinearRaw) {
            throw IOError(MODULE, "Unsupported photo metric " + std::to_string(ifd->fPhotometricInterpretation));
        }

        builder.pixelType(PixelType::RGB);

        if (ifd->fPlanarConfiguration == pcInterleaved) {
            builder.imageLayout(ImageLayout::INTERLEAVED);
        } else if (ifd->fPlanarConfiguration == pcPlanar) {
            builder.imageLayout(ImageLayout::PLANAR);
        } else {
            throw IOError(MODULE, "Unsupported planar config " + std::to_string(ifd->fPlanarConfiguration));
        }
    } else {
        throw IOError(MODULE, "Unsupported samples per pixel " + std::to_string(ifd->fSamplesPerPixel));
    }

    PixelRepresentation pixelRepresentation;
    if (ifd->fSampleFormat[0] == sfFloatingPoint) {
        pixelRepresentation = PixelRepresentation::FLOAT;
    } else if (ifd->fSampleFormat[0] == sfUnsignedInteger) {
        if (ifd->fBitsPerSample[0] > 16) {
            throw IOError(MODULE, "Unsupported bits per sample " + std::to_string(ifd->fBitsPerSample[0]));
        }
        pixelRepresentation = PixelRepresentation::UINT16;

        builder.pixelPrecision(std::ceil(std::log2(ifd->fWhiteLevel[0])));
    } else {
        throw IOError(MODULE, "Unsupported sample format " + std::to_string(ifd->fSampleFormat[0]));
    }

    mDescriptor = {builder.build(), pixelRepresentation};
}

Image16u DngReader::read16u() {
    LOG_SCOPE_F(INFO, "Read DNG (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

Imagef DngReader::readf() {
    LOG_SCOPE_F(INFO, "Read DNG (float)");
    LOG_S(INFO) << "Path: " << path();

    return read<float>();
}

template <typename T>
Image<T> DngReader::read() {
    validateType<T>();

    try {
        // Read stage 1 image from negative
        mNegative->ReadStage1Image(*mHost, *mStream, *mInfo);

        const dng_ifd *ifd = mInfo->fIFD[mInfo->fMainIndex];
        const dng_linearization_info *linearizationInfo = mNegative->GetLinearizationInfo();
        const bool hasLinearizationTable = !std::is_floating_point_v<T> && linearizationInfo &&
                                           linearizationInfo->fLinearizationTable.Get();

        Image<T> image(layoutDescriptor());

        if (!hasLinearizationTable) {
            const dng_image *stage1 = mNegative->Stage1Image();
            if (stage1->PixelType() != ttShort && stage1->PixelType() != ttFloat) {
                throw IOError(MODULE, "Unsupported pixel type " + std::to_string(stage1->PixelType()));
            }

            dng_pixel_buffer buffer(ifd->fActiveArea,
                                    0,
                                    stage1->Planes(),
                                    stage1->PixelType(),
                                    ifd->fPlanarConfiguration,
                                    image.data());
            stage1->Get(buffer);
        } else {
            LOG_S(INFO) << "Found DNG linearization table";

            const dng_image *stage1 = mNegative->Stage1Image();
            dng_const_tile_buffer srcBuffer(*stage1, stage1->Bounds());
            void *srcData = const_cast<void *>(srcBuffer.ConstPixel(0, 0, 0));

            std::vector<uint16_t> lut(linearizationInfo->fLinearizationTable->Buffer_uint16(),
                                      linearizationInfo->fLinearizationTable->Buffer_uint16() +
                                              (linearizationInfo->fLinearizationTable->LogicalSize() >> 1));

            LayoutDescriptor srcDescriptor = LayoutDescriptor::Builder(layoutDescriptor())
                                                     .width(stage1->Width())
                                                     .height(stage1->Height())
                                                     .build();
            Rect crop{ifd->fActiveArea.l, ifd->fActiveArea.t, image.width(), image.height()};

            switch (stage1->PixelType()) {
                case ttByte: {
                    ImageView8u srcImage(srcDescriptor, reinterpret_cast<uint8_t *>(srcData));
                    image = expr::lut(expr::min(srcImage[crop], lut.size() - 1), lut);
                } break;
                case ttShort: {
                    ImageView16u srcImage(srcDescriptor, reinterpret_cast<uint16_t *>(srcData));
                    image = expr::lut(expr::min(srcImage[crop], lut.size() - 1), lut);
                } break;
                default:
                    throw IOError(MODULE, "Unsupported pixel type " + std::to_string(stage1->PixelType()));
            }
        }

        return image;
    } catch (const dng_exception &except) {
        throw IOError(MODULE, "Reading failed with error code " + std::to_string(except.ErrorCode()));
    }
}

std::optional<ExifMetadata> DngReader::readExif() const {
    const dng_exif *dngExif = mNegative->GetExif();
    ExifMetadata exif;

    exif.orientation = mNegative->Orientation().GetTIFF();

    if (dngExif->fImageDescription.NotEmpty()) {
        exif.imageDescription = dngExif->fImageDescription.Get();
    }
    if (dngExif->fMake.NotEmpty()) {
        exif.make = dngExif->fMake.Get();
    }
    if (dngExif->fModel.NotEmpty()) {
        exif.model = dngExif->fModel.Get();
    }
    if (dngExif->fSoftware.NotEmpty()) {
        exif.software = dngExif->fSoftware.Get();
    }
    if (dngExif->fExposureTime.IsValid()) {
        exif.exposureTime = {dngExif->fExposureTime.n, dngExif->fExposureTime.d};
    }
    if (dngExif->fFNumber.IsValid()) {
        exif.fNumber = {dngExif->fFNumber.n, dngExif->fFNumber.d};
    }
    if (dngExif->fISOSpeedRatings[0] != 0) {
        exif.isoSpeedRatings = dngExif->fISOSpeedRatings[0];
    }
    if (dngExif->fDateTimeOriginal.IsValid()) {
        const dng_date_time &dateTime = dngExif->fDateTimeOriginal.DateTime();
        char data[20] = {0};
        snprintf(data,
                 20,
                 "%04d:%02d:%02d %02d:%02d:%02d",
                 dateTime.fYear,
                 dateTime.fMonth,
                 dateTime.fDay,
                 dateTime.fHour,
                 dateTime.fMinute,
                 dateTime.fSecond);
        exif.dateTimeOriginal = data;
    }
    if (dngExif->fBrightnessValue.IsValid()) {
        exif.brightnessValue = {dngExif->fBrightnessValue.n, dngExif->fBrightnessValue.d};
    }
    if (dngExif->fExposureBiasValue.IsValid()) {
        exif.exposureBiasValue = {dngExif->fExposureBiasValue.n, dngExif->fExposureBiasValue.d};
    }
    if (dngExif->fFocalLength.IsValid()) {
        exif.focalLength = {dngExif->fFocalLength.n, dngExif->fFocalLength.d};
    }
    if (dngExif->fFocalLengthIn35mmFilm != 0) {
        exif.focalLengthIn35mmFilm = dngExif->fFocalLengthIn35mmFilm;
    }

    return exif;
}

void DngReader::readMetadata(std::optional<ImageMetadata> &metadata) const {
    if (!metadata) {
        metadata = ImageMetadata{};
    }

    std::optional<ExifMetadata> exifMetadata = readExif();
    if (exifMetadata) {
        metadata->exifMetadata = std::move(*exifMetadata);
    }

    metadata->shootingParams.ispGain = std::exp2(mNegative->BaselineExposure());

    if (mNegative->HasCameraNeutral()) {
        const dng_vector &neutral = mNegative->CameraNeutral();
        metadata->cameraControls.whiteBalance = {static_cast<float>(1.0 / neutral[0]),
                                                 static_cast<float>(1.0 / neutral[2])};
    }

    dng_camera_profile_id profileId; // default profile ID
    dng_camera_profile profile;
    const bool haveProfile = mNegative->GetProfileByID(profileId, profile);

    if (haveProfile && profile.IsValid(mNegative->ColorChannels()) && !profile.ColorMatrix1().IsIdentity()) {
        AutoPtr<dng_color_spec> spec(mNegative->MakeColorSpec(profileId));

        if (mNegative->HasCameraNeutral()) {
            spec->SetWhiteXY(spec->NeutralToXY(mNegative->CameraNeutral()));
        } else if (mNegative->HasCameraWhiteXY()) {
            spec->SetWhiteXY(mNegative->CameraWhiteXY());
        } else {
            spec->SetWhiteXY(D55_xy_coord());
        }

        const dng_vector &neutral = spec->CameraWhite();
        if (mNegative->HasCameraWhiteXY() && !mNegative->HasCameraNeutral()) {
            metadata->cameraControls.whiteBalance = {static_cast<float>(1.0 / neutral[0]),
                                                     static_cast<float>(1.0 / neutral[2])};
        }

        // Compute camera to sRGB color matrix
        dng_matrix cameraToRGB = dng_space_sRGB::Get().MatrixFromPCS() * spec->CameraToPCS();

        metadata->calibrationData.colorMatrix = Matrix3{};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                (*metadata->calibrationData.colorMatrix)(i, j) = cameraToRGB[i][j] * neutral[j];
            }
        }
    }

    const dng_linearization_info *linearizationInfo = mNegative->GetLinearizationInfo();
    if (linearizationInfo) {
        if (mNegative->IsFloatingPoint()) {
            metadata->calibrationData.blackLevel = static_cast<float>(linearizationInfo->MaxBlackLevel(0));
        } else {
            metadata->calibrationData.blackLevel = static_cast<int>(std::lround(linearizationInfo->MaxBlackLevel(0)));
            metadata->calibrationData.whiteLevel = static_cast<int>(std::lround(linearizationInfo->fWhiteLevel[0]));
        }
    }

    std::unordered_map<Bayer, const dng_gain_map *> gainMaps;

    mNegative->ReadOpcodeLists(*mHost, *mStream, *mInfo);
    for (unsigned i = 0; i < mNegative->OpcodeList2().Count(); ++i) {
        const dng_opcode &opcode = mNegative->OpcodeList2().Entry(i);
        switch (opcode.OpcodeID()) {
            case dngOpcode_GainMap: {
                if (!model::isBayerPixelType(layoutDescriptor().pixelType)) {
                    continue;
                }

                const auto &gainMapOpcode = static_cast<const dng_opcode_GainMap &>(opcode);
                const dng_gain_map &gainMap = gainMapOpcode.GainMap();
                const dng_rect &area = gainMapOpcode.AreaSpec().Area();

                if (area.l == model::bayerOffsetX(layoutDescriptor().pixelType, Bayer::R) &&
                    area.t == model::bayerOffsetY(layoutDescriptor().pixelType, Bayer::R)) {
                    gainMaps[Bayer::R] = &gainMap;
                } else if (area.l == model::bayerOffsetX(layoutDescriptor().pixelType, Bayer::GR) &&
                           area.t == model::bayerOffsetY(layoutDescriptor().pixelType, Bayer::GR)) {
                    gainMaps[Bayer::GR] = &gainMap;
                } else if (area.l == model::bayerOffsetX(layoutDescriptor().pixelType, Bayer::GB) &&
                           area.t == model::bayerOffsetY(layoutDescriptor().pixelType, Bayer::GB)) {
                    gainMaps[Bayer::GB] = &gainMap;
                } else if (area.l == model::bayerOffsetX(layoutDescriptor().pixelType, Bayer::B) &&
                           area.t == model::bayerOffsetY(layoutDescriptor().pixelType, Bayer::B)) {
                    gainMaps[Bayer::B] = &gainMap;
                } else {
                    continue;
                }
            } break;

            default:
                break;
        }
    }

    if (gainMaps.size() == 4) {
        const auto &gainMapR = *gainMaps[Bayer::R];
        const auto &gainMapGR = *gainMaps[Bayer::GR];
        const auto &gainMapGB = *gainMaps[Bayer::GB];
        const auto &gainMapB = *gainMaps[Bayer::B];

        if (gainMapR.Points().h == gainMapGR.Points().h && gainMapR.Points().h == gainMapGB.Points().h &&
            gainMapR.Points().h == gainMapB.Points().h && gainMapR.Points().v == gainMapGR.Points().v &&
            gainMapR.Points().v == gainMapGB.Points().v && gainMapR.Points().v == gainMapB.Points().v) {
            DynamicMatrix vignetting(gainMapR.Points().v, gainMapR.Points().h);
            DynamicMatrix colorR(gainMapR.Points().v, gainMapR.Points().h);
            DynamicMatrix colorB(gainMapR.Points().v, gainMapR.Points().h);
            for (int y = 0; y < gainMapR.Points().v; ++y) {
                for (int x = 0; x < gainMapR.Points().h; ++x) {
                    vignetting(y, x) = 0.5f * (gainMapGR.Entry(y, x, 0) + gainMapGB.Entry(y, x, 0));
                    colorR(y, x) = gainMapR.Entry(y, x, 0) / vignetting(y, x);
                    colorB(y, x) = gainMapB.Entry(y, x, 0) / vignetting(y, x);
                }
            }

            metadata->calibrationData.vignetting = std::move(vignetting);
            metadata->cameraControls.colorShading = {std::move(colorR), std::move(colorB)};
        }
    }

    for (unsigned i = 0; i < mNegative->NumSemanticMasks(); ++i) {
        const auto &dngSemanticMask = mNegative->SemanticMask(i);
        ImageMetadata::SemanticMask semanticMask{
                .name = dngSemanticMask.fName.Get(),
                .label = SemanticLabel::UNKNOWN,
                .mask = DynamicMatrix(dngSemanticMask.fMask->Height(), dngSemanticMask.fMask->Width())};

        dng_rect bound(dngSemanticMask.fMask->Width(), dngSemanticMask.fMask->Height());
        switch (dngSemanticMask.fMask->PixelType()) {
            case ttFloat: {
                dng_pixel_buffer maskBuffer(bound, 0, 1, ttFloat, pcPlanar, semanticMask.mask.data());
                dngSemanticMask.fMask->Get(maskBuffer);
            } break;
        }

        metadata->semanticMasks.emplace(std::make_pair(semanticMask.label, std::move(semanticMask)));
    }
}

static void populateExif(dng_exif *dngExif, const ExifMetadata &exif) {
    dngExif->SetVersion0231();

    if (exif.imageDescription) {
        dngExif->fImageDescription.Set(exif.imageDescription->c_str());
    }
    if (exif.make) {
        dngExif->fMake.Set(exif.make->c_str());
    }
    if (exif.model) {
        dngExif->fModel.Set(exif.model->c_str());
    }
    if (exif.software) {
        dngExif->fSoftware.Set(exif.software->c_str());
    }
    if (exif.exposureTime) {
        dngExif->fExposureTime = {exif.exposureTime->numerator, exif.exposureTime->denominator};
    }
    if (exif.fNumber) {
        dngExif->fFNumber = {exif.fNumber->numerator, exif.fNumber->denominator};
    }
    if (exif.isoSpeedRatings) {
        dngExif->fISOSpeedRatings[0] = *exif.isoSpeedRatings;
    }
    if (exif.dateTimeOriginal) {
        dng_date_time dateTime;
        if (dateTime.Parse(exif.dateTimeOriginal->c_str())) {
            dngExif->fDateTimeOriginal.SetDateTime(dateTime);
        }
    }
    if (exif.brightnessValue) {
        dngExif->fBrightnessValue = {exif.brightnessValue->numerator, exif.brightnessValue->denominator};
    }
    if (exif.exposureBiasValue) {
        dngExif->fExposureBiasValue = {exif.exposureBiasValue->numerator, exif.exposureBiasValue->denominator};
    }
    if (exif.focalLength) {
        dngExif->fFocalLength = {exif.focalLength->numerator, exif.focalLength->denominator};
    }
    if (exif.focalLengthIn35mmFilm) {
        dngExif->fFocalLengthIn35mmFilm = *exif.focalLengthIn35mmFilm;
    }
}

void DngWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write DNG (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint16_t>(image);
}

void DngWriter::write(const Imagef &image) const {
    LOG_SCOPE_F(INFO, "Write DNG (float)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<float>(image);
}

template <typename T>
void DngWriter::writeImpl(const Image<T> &image) const {
    if (image.imageLayout() == ImageLayout::PLANAR && image.numPlanes() > 1) {
        // Planar to interleaved conversion
        return writeImpl<T>(image::convertLayout(image, ImageLayout::INTERLEAVED));
    }

    try {
        dng_host host;

        const uint32 pixelType = std::is_floating_point_v<T> ? ttFloat : ttShort;
        AutoPtr<dng_image> stage1(
                host.Make_dng_image(dng_rect(image.height(), image.width()), image.numPlanes(), pixelType));

        dng_pixel_buffer buffer(stage1->Bounds(),
                                0,
                                stage1->Planes(),
                                stage1->PixelType(),
                                pcInterleaved,
                                const_cast<void *>(reinterpret_cast<const void *>(image.data())));
        stage1->Put(buffer);

        AutoPtr<dng_negative> negative(host.Make_dng_negative());
        negative->SetFloatingPoint(std::is_floating_point_v<T>);
        negative->SetRGB();

        switch (image.pixelType()) {
            case PixelType::BAYER_BGGR:
            case PixelType::QUADBAYER_BGGR:
                negative->SetBayerMosaic(2);
                break;
            case PixelType::BAYER_GBRG:
            case PixelType::QUADBAYER_GBRG:
                negative->SetBayerMosaic(3);
                break;
            case PixelType::BAYER_GRBG:
            case PixelType::QUADBAYER_GRBG:
                negative->SetBayerMosaic(0);
                break;
            case PixelType::BAYER_RGGB:
            case PixelType::QUADBAYER_RGGB:
                negative->SetBayerMosaic(1);
                break;
            case PixelType::RGB:
                break;
            default:
                throw IOError(MODULE, "Unsupported pixel type "s + toString(image.pixelType()));
        }

        AutoPtr<dng_camera_profile> profile(new dng_camera_profile());

        const auto &options = this->options();
        if (options.metadata) {
            const ImageMetadata &metadata = *options.metadata;

            if (metadata.cameraControls.whiteBalance) {
                dng_vector neutral(3);
                neutral[0] = 1.0 / metadata.cameraControls.whiteBalance->gainR;
                neutral[1] = 1.0;
                neutral[2] = 1.0 / metadata.cameraControls.whiteBalance->gainB;
                negative->SetCameraNeutral(neutral);
            }
            if (metadata.exifMetadata.orientation) {
                dng_orientation orientation;
                orientation.SetTIFF(*metadata.exifMetadata.orientation);

                negative->SetBaseOrientation(orientation);
            }
            if (metadata.shootingParams.ispGain) {
                negative->SetBaselineExposure(std::log2(*metadata.shootingParams.ispGain));
            }
            if (metadata.calibrationData.blackLevel) {
                double blackLevel = std::is_floating_point_v<T> ? std::get<float>(*metadata.calibrationData.blackLevel)
                                                                : std::get<int>(*metadata.calibrationData.blackLevel);
                negative->SetBlackLevel(blackLevel);
            }
            if (metadata.calibrationData.whiteLevel && !std::is_floating_point_v<T>) {
                negative->SetWhiteLevel(std::get<int>(*metadata.calibrationData.whiteLevel));
            }
            if (metadata.calibrationData.colorMatrix && metadata.cameraControls.whiteBalance) {
                Matrix3 xyzToCamera = metadata.calibrationData.colorMatrix->inverse() *
                                      colorspace::transformationMatrix(
                                              RgbColorSpace::XYZ_D50,
                                              metadata.calibrationData.colorMatrixTarget.value_or(RgbColorSpace::SRGB));

                dng_matrix colorMatrix(3, 3);
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        colorMatrix[i][j] = xyzToCamera(i, j) * negative->CameraNeutral()[i];
                    }
                }
                profile->SetColorMatrix1(colorMatrix);
            }
            if (metadata.calibrationData.vignetting || metadata.cameraControls.colorShading) {
                int numRows = 0;
                int numCols = 0;
                bool valid = true;

                const auto checkMatrix = [&](const DynamicMatrix &map) {
                    if (numRows == 0 && numCols == 0) {
                        numRows = map.numRows();
                        numCols = map.numCols();
                    } else if (numRows != map.numRows() || numCols != map.numCols()) {
                        valid = false;
                    }
                };

                if (metadata.calibrationData.vignetting) {
                    checkMatrix(*metadata.calibrationData.vignetting);
                }

                if (metadata.cameraControls.colorShading) {
                    checkMatrix(metadata.cameraControls.colorShading->gainR);
                    checkMatrix(metadata.cameraControls.colorShading->gainB);
                }

                if (numRows > 0 && numCols > 0 && valid) {
                    const Bayer bayers[] = {Bayer::R, Bayer::GR, Bayer::GB, Bayer::B};
                    for (auto bayer : bayers) {
                        AutoPtr<dng_gain_map> gainMap(new dng_gain_map(host.Allocator(),
                                                                       {numRows, numCols},
                                                                       {1.0 / (numRows - 1), 1.0 / (numCols - 1)},
                                                                       {0.0, 0.0},
                                                                       1));

                        for (int y = 0; y < numRows; ++y) {
                            for (int x = 0; x < numCols; ++x) {
                                float gain = 1.0f;
                                if (metadata.calibrationData.vignetting) {
                                    gain = (*metadata.calibrationData.vignetting)(y, x);
                                }
                                if (bayer == Bayer::R && metadata.cameraControls.colorShading) {
                                    gain *= metadata.cameraControls.colorShading->gainR(y, x);
                                } else if (bayer == Bayer::B && metadata.cameraControls.colorShading) {
                                    gain *= metadata.cameraControls.colorShading->gainB(y, x);
                                }

                                gainMap->Entry(y, x, 0) = gain;
                            }
                        }

                        AutoPtr<dng_opcode> opcode(
                                new dng_opcode_GainMap({{model::bayerOffsetY(image.pixelType(), bayer),
                                                         model::bayerOffsetX(image.pixelType(), bayer),
                                                         image.height(),
                                                         image.width()},
                                                        0,
                                                        1,
                                                        2,
                                                        2},
                                                       gainMap));
                        negative->OpcodeList2().Append(opcode);
                    }
                }
            }

            populateExif(negative->GetExif(), metadata.exifMetadata);
        }

        negative->AddProfile(profile);
        negative->SetStage1Image(stage1);
        negative->SynchronizeMetadata();

        DngWriteStream writeStream(mStream);
        dng_image_writer writer;
        writer.WriteDNG(host, writeStream, *negative.Get());
    } catch (const dng_exception &except) {
        throw IOError(MODULE, "Writing failed with error code " + std::to_string(except.ErrorCode()));
    }
}

}; // namespace cxximg
