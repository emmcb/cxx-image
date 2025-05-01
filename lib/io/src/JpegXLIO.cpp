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

#include "JpegXLIO.h"

#ifdef HAVE_EXIF
#include "Exif.h"
#endif

#include <jxl/decode.h>
#include <jxl/encode_cxx.h>
#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "JPEGXL";

void JxlDecoderDeleter::operator()(JxlDecoder *decoder) const {
    JxlDecoderDestroy(decoder);
}

void JpegXLReader::initialize() {
    mDecoder.reset(JxlDecoderCreate(nullptr));
    JxlDecoderSetKeepOrientation(mDecoder.get(), JXL_TRUE);
    const bool supportBoxDecompression = (JxlDecoderSetDecompressBoxes(mDecoder.get(), JXL_TRUE) == JXL_DEC_SUCCESS);

    JxlDecoderSubscribeEvents(
            mDecoder.get(),
            JXL_DEC_BASIC_INFO | JXL_DEC_BOX | JXL_DEC_BOX_COMPLETE | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE);

    JxlBasicInfo info;
    size_t exifPos = 0;

    while (true) {
        mStream->read(reinterpret_cast<char *>(mBuffer.data() + mRemainingBytes), CHUNK_SIZE - mRemainingBytes);
        const std::streamsize bytesRead = mStream->gcount();

        if (bytesRead <= 0 && mRemainingBytes == 0) {
            throw IOError(MODULE, "Unexpected end of stream");
        }
        const size_t availableBytes = mRemainingBytes + bytesRead;

        JxlDecoderSetInput(mDecoder.get(), mBuffer.data(), availableBytes);
        JxlDecoderStatus status = JxlDecoderProcessInput(mDecoder.get());

        mRemainingBytes = JxlDecoderReleaseInput(mDecoder.get());
        if (mRemainingBytes > 0) {
            // Move the remaining unused data to the beginning of the buffer
            memmove(mBuffer.data(), mBuffer.data() + (availableBytes - mRemainingBytes), mRemainingBytes);
        }

        if (status == JXL_DEC_ERROR) {
            throw IOError(MODULE, "Decoder error");
        }
        if (status == JXL_DEC_BOX) {
            JxlBoxType type;
            JxlDecoderGetBoxType(mDecoder.get(), type, TO_JXL_BOOL(supportBoxDecompression));

            if (memcmp(type, "Exif", 4) == 0) {
                mExif.resize(CHUNK_SIZE);
                JxlDecoderSetBoxBuffer(mDecoder.get(), mExif.data(), mExif.size());
            }
        } else if (status == JXL_DEC_BOX_NEED_MORE_OUTPUT) {
            const size_t remaining = JxlDecoderReleaseBoxBuffer(mDecoder.get());
            exifPos += CHUNK_SIZE - remaining;
            mExif.resize(mExif.size() + CHUNK_SIZE);
            JxlDecoderSetBoxBuffer(mDecoder.get(), mExif.data() + exifPos, mExif.size() - exifPos);
        } else if (status == JXL_DEC_BOX_COMPLETE) {
            const size_t remaining = JxlDecoderReleaseBoxBuffer(mDecoder.get());
            mExif.resize(mExif.size() - remaining);
        } else if (status == JXL_DEC_BASIC_INFO) {
            JxlDecoderGetBasicInfo(mDecoder.get(), &info);
        } else if (status == JXL_DEC_FRAME) {
            break; // break at the beginning of the first frame
        } else if (status != JXL_DEC_NEED_MORE_INPUT) {
            throw IOError(MODULE, "Unexpected decoder status: " + std::to_string(status));
        }
    }

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(info.xsize, info.ysize)
                                                .pixelPrecision(info.bits_per_sample);

    if (info.num_color_channels == 1 && info.num_extra_channels == 0) {
        builder.pixelType(PixelType::GRAYSCALE);
    } else if (info.num_color_channels == 1 && info.num_extra_channels == 1) {
        builder.imageLayout(ImageLayout::INTERLEAVED).pixelType(PixelType::GRAY_ALPHA);
    } else if (info.num_color_channels == 3 && info.num_extra_channels == 0) {
        builder.imageLayout(ImageLayout::INTERLEAVED).pixelType(PixelType::RGB);
    } else if (info.num_color_channels == 3 && info.num_extra_channels == 1) {
        builder.imageLayout(ImageLayout::INTERLEAVED).pixelType(PixelType::RGBA);
    } else {
        throw IOError(MODULE,
                      "Unsupported combination of color channels: " + std::to_string(info.num_color_channels) +
                              " and extra channels: " + std::to_string(info.num_extra_channels));
    }

    PixelRepresentation pixelRepresentation = [&]() {
        if (info.bits_per_sample == 8 && info.exponent_bits_per_sample == 0) {
            return PixelRepresentation::UINT8;
        }
        if (info.bits_per_sample == 16 && info.exponent_bits_per_sample == 0) {
            return PixelRepresentation::UINT16;
        }
        if (info.bits_per_sample == 32 && info.exponent_bits_per_sample == 8) {
            return PixelRepresentation::FLOAT;
        }
        throw IOError(MODULE, "Unsupported bit depth: " + std::to_string(info.bits_per_sample));
    }();

    mDescriptor = {builder.build(), pixelRepresentation};
}

Image8u JpegXLReader::read8u() {
    LOG_SCOPE_F(INFO, "Read JPEG XL (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint8_t>();
}

Image16u JpegXLReader::read16u() {
    LOG_SCOPE_F(INFO, "Read JPEG XL (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

Imagef JpegXLReader::readf() {
    LOG_SCOPE_F(INFO, "Read JPEG XL (float)");
    LOG_S(INFO) << "Path: " << path();

    return read<float>();
}

template <typename T>
Image<T> JpegXLReader::read() {
    validateType<T>();

    const auto jxlDataType = [](PixelRepresentation pixelRepresentation) {
        switch (pixelRepresentation) {
            case PixelRepresentation::UINT8:
                return JXL_TYPE_UINT8;
            case PixelRepresentation::UINT16:
                return JXL_TYPE_UINT16;
            case PixelRepresentation::FLOAT:
                return JXL_TYPE_FLOAT;
            default:
                throw IOError(MODULE, "Unsupported pixel representation: "s + toString(pixelRepresentation));
        }
    };
    JxlPixelFormat format = {static_cast<uint32_t>(layoutDescriptor().numPlanes),
                             jxlDataType(pixelRepresentation()),
                             JXL_NATIVE_ENDIAN,
                             0};

    Image<T> image(layoutDescriptor());
    while (true) {
        mStream->read(reinterpret_cast<char *>(mBuffer.data() + mRemainingBytes), CHUNK_SIZE - mRemainingBytes);
        std::streamsize bytesRead = mStream->gcount();

        if (bytesRead <= 0 && mRemainingBytes == 0) {
            throw IOError(MODULE, "Unexpected end of stream");
        }
        size_t availableBytes = mRemainingBytes + bytesRead;

        JxlDecoderSetInput(mDecoder.get(), mBuffer.data(), availableBytes);
        JxlDecoderStatus status = JxlDecoderProcessInput(mDecoder.get());

        mRemainingBytes = JxlDecoderReleaseInput(mDecoder.get());
        if (mRemainingBytes > 0) {
            // Move the remaining unused data to the beginning of the buffer
            memmove(mBuffer.data(), mBuffer.data() + (availableBytes - mRemainingBytes), mRemainingBytes);
        }

        if (status == JXL_DEC_ERROR) {
            throw IOError(MODULE, "Decoder error");
        }
        if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t bufferSize = 0;
            JxlDecoderImageOutBufferSize(mDecoder.get(), &format, &bufferSize);

            if (bufferSize != image.size() * sizeof(T)) {
                throw IOError(MODULE,
                              "Buffer size does not match expected image size (expected " +
                                      std::to_string(image.size() * sizeof(T)) + ", got " + std::to_string(bufferSize) +
                                      ")");
            }

            JxlDecoderSetImageOutBuffer(mDecoder.get(), &format, image.data(), bufferSize);
        } else if (status == JXL_DEC_FULL_IMAGE) {
            return image;
        } else if (status != JXL_DEC_NEED_MORE_INPUT) {
            throw IOError(MODULE, "Unexpected decoder status: " + std::to_string(status));
        }
    }
}

#ifdef HAVE_EXIF
std::optional<ExifMetadata> JpegXLReader::readExif() const {
    if (mExif.empty()) {
        return std::nullopt;
    }

    return detail::readExif(mExif.data(), mExif.size());
}
#endif

void JpegXLWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write JPEG XL (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint8_t>(image);
}

void JpegXLWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write JPEG XL (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint16_t>(image);
}

void JpegXLWriter::write(const Imagef &image) const {
    LOG_SCOPE_F(INFO, "Write JPEG XL (float)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<float>(image);
}

template <typename T>
void JpegXLWriter::writeImpl(const Image<T> &image) const {
    if (image.imageLayout() == ImageLayout::PLANAR && image.numPlanes() > 1) {
        // Planar to interleaved conversion
        return writeImpl<T>(image::convertLayout(image, ImageLayout::INTERLEAVED));
    }

    JxlEncoderPtr encoder = JxlEncoderMake(nullptr);

    const auto jxlDataType = []() {
        if constexpr (std::is_same_v<T, uint8_t>) {
            return JXL_TYPE_UINT8;
        }
        if constexpr (std::is_same_v<T, uint16_t>) {
            return JXL_TYPE_UINT16;
        }
        if constexpr (std::is_same_v<T, float>) {
            return JXL_TYPE_FLOAT;
        }
    }();

    JxlPixelFormat format = {static_cast<uint32_t>(image.numPlanes()), jxlDataType, JXL_NATIVE_ENDIAN, 0};
    const bool isGray = format.num_channels < 3;

    JxlBasicInfo info;
    JxlEncoderInitBasicInfo(&info);
    info.xsize = image.width();
    info.ysize = image.height();
    info.bits_per_sample = sizeof(T) * 8;
    if constexpr (std::is_same_v<T, float>) {
        info.exponent_bits_per_sample = 8;
    }
    if (isGray) {
        info.num_color_channels = 1;
    }
    if (format.num_channels == 2 || format.num_channels == 4) {
        info.num_extra_channels = 1;
    }
    JxlEncoderSetBasicInfo(encoder.get(), &info);

    JxlColorEncoding colorEncoding = {};
    if constexpr (std::is_same_v<T, uint8_t>) {
        JxlColorEncodingSetToSRGB(&colorEncoding, TO_JXL_BOOL(isGray));
    } else {
        JxlColorEncodingSetToLinearSRGB(&colorEncoding, TO_JXL_BOOL(isGray));
    }
    JxlEncoderSetColorEncoding(encoder.get(), &colorEncoding);

#ifdef HAVE_EXIF
    // Write EXIF
    const auto &metadata = options().metadata;
    if (metadata) {
        ExifMem *mem = exif_mem_new_default();
        ExifData *data = exif_data_new();

        detail::populateExif(mem, data, metadata->exifMetadata);

        uint8_t *exifBuffer = nullptr;
        uint32_t exifLength = 0;
        exif_data_save_data(data, &exifBuffer, &exifLength);

        JxlEncoderUseBoxes(encoder.get());
        JxlEncoderAddBox(encoder.get(), "Exif", exifBuffer, exifLength, JXL_FALSE);

        free(exifBuffer); // NOLINT(cppcoreguidelines-no-malloc)
        exif_mem_unref(mem);
        exif_data_unref(data);
    }
#endif

    JxlEncoderFrameSettings *frameSettings = JxlEncoderFrameSettingsCreate(encoder.get(), nullptr);

    const float distance = JxlEncoderDistanceFromQuality(options().jpegQuality);
    JxlEncoderSetFrameDistance(frameSettings, distance);

    JxlEncoderFrameSettingsSetOption(frameSettings, JXL_ENC_FRAME_SETTING_EFFORT, options().compressionLevel);

    JxlEncoderAddImageFrame(frameSettings, &format, static_cast<const void *>(image.data()), image.size() * sizeof(T));
    JxlEncoderCloseInput(encoder.get());

    std::array<uint8_t, CHUNK_SIZE> buffer = {};
    while (true) {
        uint8_t *nextOut = buffer.data();
        size_t availOut = CHUNK_SIZE;

        JxlEncoderStatus status = JxlEncoderProcessOutput(encoder.get(), &nextOut, &availOut);

        if (status == JXL_ENC_ERROR) {
            throw IOError(MODULE, "Encoder error");
        }

        mStream->write(reinterpret_cast<const char *>(buffer.data()), CHUNK_SIZE - availOut);

        if (status == JXL_ENC_SUCCESS) {
            break;
        }
        if (status != JXL_ENC_NEED_MORE_OUTPUT) {
            throw IOError(MODULE, "Unexpected encoder status: " + std::to_string(status));
        }
    }
}

} // namespace cxximg
