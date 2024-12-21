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

#include <fstream>
#include <optional>
#include <string>

namespace cxximg {

/// Abstract image writer class.
/// @ingroup io
class ImageWriter {
public:
    enum class TiffCompression { NONE, DEFLATE };

    struct Options {
        std::optional<FileFormat> fileFormat;
        std::optional<ImageMetadata> metadata;

        int jpegQuality = 95;
        TiffCompression tiffCompression = TiffCompression::DEFLATE;

        Options() = default;

        explicit Options(const std::optional<ImageMetadata> &metadata) {
            if (metadata) {
                this->metadata = metadata;
            }
        }
    };

    static std::optional<TiffCompression> parseTiffCompression(const std::string &tiffCompression) {
        if (tiffCompression == "none") {
            return TiffCompression::NONE;
        }
        if (tiffCompression == "deflate") {
            return TiffCompression::DEFLATE;
        }
        return std::nullopt;
    }

    /// Constructs with stream and options.
    ImageWriter(std::string path, std::ostream *stream, Options options)
        : mStream(stream), mPath(std::move(path)), mOptions(std::move(options)) {
        if (!stream) {
            mOwnStream = std::make_unique<std::ofstream>(mPath, std::ios::binary);
            mStream = mOwnStream.get();

            if (!*mStream) {
                throw IOError("Cannot open file for writing: " + mPath);
            }
        }
    }

    /// Destructor.
    virtual ~ImageWriter() = default;

    /// Check if the writer can write the given image descriptor.
    virtual bool acceptDescriptor(const LayoutDescriptor &descriptor) const = 0;

    /// Encode and write the given 8 bits image into a file.
    virtual void write([[maybe_unused]] const Image8u &image) const {
        throw IOError("This format does not support 8 bits write.");
    }

    /// Encode and write the given 16 bits image into a file.
    virtual void write([[maybe_unused]] const Image16u &image) const {
        throw IOError("This format does not support 16 bits write.");
    }

    /// Encode and write the given float image into a file.
    virtual void write([[maybe_unused]] const Imagef &image) const {
        throw IOError("This format does not support float write.");
    }

    /// Write the given EXIF metadata into a file.
    virtual void writeExif([[maybe_unused]] const ExifMetadata &exif) const {
        throw IOError("This format does not support EXIF write.");
    }

    /// Encode and write the given view into a file.
    template <typename T>
    void write(const ImageView<T> &imageView) const {
        return write(image::clone(imageView));
    }

protected:
    const std::string &path() const { return mPath; }
    const Options &options() const { return mOptions; }

    std::ostream *mStream;

private:
    std::string mPath;
    Options mOptions;

    std::unique_ptr<std::ostream> mOwnStream;
};

} // namespace cxximg
