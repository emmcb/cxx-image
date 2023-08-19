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

#pragma once

#include "io/Exceptions.h"

#include "image/Image.h"
#include "model/ExifMetadata.h"
#include "model/ImageMetadata.h"

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

    /// Read and decode the given file into a newly allocated 8 bits image.
    virtual Image8u read8u() { throw IOError("This format does not support 8 bits read."); }

    /// Read and decode the given file into a newly allocated 16 bits image.
    virtual Image16u read16u() { throw IOError("This format does not support 16 bits read."); }

    /// Read and decode the given file into a newly allocated float image.
    virtual Imagef readf() { throw IOError("This format does not support float read."); }

    /// Read the file EXIF metadata, if present.
    virtual std::optional<ExifMetadata> readExif() const { return std::nullopt; }

    /// Read the file metadata and updates the given structure with the result.
    virtual void readMetadata(std::optional<ImageMetadata>& metadata) const {
        std::optional<ExifMetadata> exif = readExif();
        if (exif) {
            if (!metadata) {
                metadata = ImageMetadata{};
            }
            metadata->exifMetadata = std::move(*exif);
        }
    }

protected:
    struct Descriptor final {
        Descriptor() = delete;

        LayoutDescriptor layout;
        PixelRepresentation pixelRepresentation;
    };

    /// Constructs with file path and options.
    ImageReader(std::string mPath, Options options) : mPath(std::move(mPath)), mOptions(options) {}

    const std::string& path() const noexcept { return mPath; }
    const Options& options() const noexcept { return mOptions; }

    void setDescriptor(const Descriptor& descriptor) noexcept { mDescriptor = descriptor; }

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

private:
    std::string mPath;
    Options mOptions;

    std::optional<Descriptor> mDescriptor;
};

} // namespace cxximg
