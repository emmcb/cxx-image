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

#pragma once

#include "cxximg/io/Exceptions.h"

#include "cxximg/image/Image.h"
#include "cxximg/model/ExifMetadata.h"
#include "cxximg/model/ImageMetadata.h"

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace cxximg {

/// Abstract image reader class.
/// @ingroup io
class ImageReader {
public:
    enum class JpegDecodingMode { YUV, RGB };

    struct Options final {
        ImageMetadata::FileInfo fileInfo;
        JpegDecodingMode jpegDecodingMode = JpegDecodingMode::RGB;

        Options() = default;

        explicit Options(const std::optional<ImageMetadata>& metadata) {
            if (metadata) {
                fileInfo = metadata->fileInfo;
            }
        }
    };

    /// Constructs with stream and options.
    ImageReader(std::string path, std::istream* stream, Options options)
        : mStream(stream), mPath(std::move(path)), mOptions(options) {
        if (!stream) {
            mOwnStream = std::make_unique<std::ifstream>(mPath, std::ios::binary);
            mStream = mOwnStream.get();

            if (!*mStream) {
                throw IOError("Cannot open file for reading: " + mPath);
            }
        }
    }

    /// Destructor.
    virtual ~ImageReader() = default;

    /// Returns the image pixel representation.
    PixelRepresentation pixelRepresentation() const noexcept {
        assert(mDescriptor.has_value());
        return mDescriptor->pixelRepresentation;
    }

    /// Returns the image layout descriptor.
    LayoutDescriptor layoutDescriptor() const noexcept {
        assert(mDescriptor.has_value());
        return mDescriptor->layout;
    }

    /// Initialize the reader.
    /// Implementations must read the image header and fill descriptor required values.
    virtual void initialize() = 0;

    /// Read and decode the opened stream into a newly allocated 8 bits image.
    virtual Image8u read8u() { throw IOError("This format does not support 8 bits read."); }

    /// Read and decode the opened stream into a newly allocated 16 bits image.
    virtual Image16u read16u() { throw IOError("This format does not support 16 bits read."); }

    /// Read and decode the opened stream into a newly allocated float image.
    virtual Imagef readf() { throw IOError("This format does not support float read."); }

    /// Read the image EXIF metadata, if available.
    virtual std::optional<ExifMetadata> readExif() const { return std::nullopt; }

    /// Read the image metadata if available and updates the given structure with the result.
    virtual void readMetadata(std::optional<ImageMetadata>& metadata) const {
        std::optional<ExifMetadata> exif = readExif();
        if (exif) {
            if (!metadata) {
                metadata = ImageMetadata{};
            }
            metadata->exifMetadata = std::move(*exif);
        }
    }

    /// Read the image metadata, if available.
    std::optional<ImageMetadata> readMetadata() const {
        std::optional<ImageMetadata> metadata;
        readMetadata(metadata);

        return metadata;
    }

protected:
    struct Descriptor final {
        Descriptor(const LayoutDescriptor& layout_, PixelRepresentation pixelRepresentation_)
            : layout(layout_), pixelRepresentation(pixelRepresentation_) {}

        LayoutDescriptor layout;
        PixelRepresentation pixelRepresentation;
    };

    const std::string& path() const { return mPath; }
    const Options& options() const { return mOptions; }

    template <typename T>
    void validateType() const {
        using namespace std::string_literals;

        if constexpr (std::is_same_v<T, uint8_t>) {
            if (pixelRepresentation() != PixelRepresentation::UINT8) {
                throw IOError("Attempting to read "s + toString(pixelRepresentation()) + " image as uint8.");
            }
        }

        else if constexpr (std::is_same_v<T, uint16_t>) {
            if (pixelRepresentation() != PixelRepresentation::UINT16) {
                throw IOError("Attempting to read "s + toString(pixelRepresentation()) + " image as uint16.");
            }
        }

        else if constexpr (std::is_same_v<T, float>) {
            if (pixelRepresentation() != PixelRepresentation::FLOAT) {
                throw IOError("Attempting to read "s + toString(pixelRepresentation()) + " image as float.");
            }
        }
    }

    std::istream* mStream;
    std::optional<Descriptor> mDescriptor;

private:
    std::string mPath;
    Options mOptions;

    std::unique_ptr<std::istream> mOwnStream;
};

} // namespace cxximg
