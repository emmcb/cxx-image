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

#include "PlainIO.h"
#include "Alignment.h"

#include <loguru.hpp>

#include <optional>
#include <tuple>

namespace cxximg {

static const std::string MODULE = "PLAIN";

static decltype(auto) guessPixelFromExtension(const std::string &path) {
    std::string extension = file::extension(path);

    std::optional<ImageLayout> imageLayout;
    std::optional<PixelType> pixelType;

    if (extension == "nv12") {
        LOG_S(INFO) << "Guess imageLayout NV12 for file extension nv12";
        imageLayout = ImageLayout::NV12;
    } else if (extension == "y8") {
        LOG_S(INFO) << "Guess pixelType GRAYSCALE for file extension y8";
        pixelType = PixelType::GRAYSCALE;
    }

    return std::make_tuple(imageLayout, pixelType);
}

void PlainReader::readHeader() {
    mStream->seekg(0, std::istream::end);
    int64_t fileSize = mStream->tellg();
    mStream->seekg(0);

    const auto &fileInfo = options().fileInfo;
    if (!fileInfo.width || !fileInfo.height) {
        throw IOError(MODULE, "Unspecified image dimensions");
    }

    const int width = *fileInfo.width;
    const int height = *fileInfo.height;
    const auto [imageLayout, pixelType] = guessPixelFromExtension(path());

    LayoutDescriptor::Builder builder = LayoutDescriptor::Builder(width, height);
    if (fileInfo.imageLayout || imageLayout) {
        builder.imageLayout(fileInfo.imageLayout.value_or(*imageLayout));
    }
    if (fileInfo.pixelType || pixelType) {
        builder.pixelType(fileInfo.pixelType.value_or(*pixelType));
    }
    if (fileInfo.pixelPrecision) {
        builder.pixelPrecision(*fileInfo.pixelPrecision);
    }

    builder.widthAlignment([&]() -> int {
        if (fileInfo.widthAlignment) {
            return *fileInfo.widthAlignment;
        }

        // Look for the width alignment that matches with the file size.
        std::optional<int> widthAlignment = detail::guessWidthAlignment(builder, fileSize);
        if (!widthAlignment) {
            throw IOError(
                    MODULE,
                    "Cannot guess relevant width alignment corresponding to file size " + std::to_string(fileSize));
        }

        LOG_S(INFO) << "Guess width alignment " << *widthAlignment << " from file size " << fileSize << ".";
        return *widthAlignment;
    }());

    LayoutDescriptor layout = builder.build();
    if (layout.pixelType == PixelType::CUSTOM) {
        throw IOError(MODULE, "Unspecified pixel type");
    }

    PixelRepresentation pixelRepresentation = [&]() {
        if (fileInfo.pixelRepresentation) {
            return *fileInfo.pixelRepresentation;
        }

        // Look for pixel size that can fit with file size.
        const int pixelSize = detail::guessPixelSize(builder, fileSize);

        if (pixelSize == 1) {
            return PixelRepresentation::UINT8;
        }
        if (pixelSize == 2) {
            return PixelRepresentation::UINT16;
        }
        if (pixelSize == 4) {
            return PixelRepresentation::FLOAT;
        }
        throw IOError(MODULE, "Unsupported pixel size " + std::to_string(pixelSize));
    }();

    mDescriptor = {layout, pixelRepresentation};
}

Image8u PlainReader::read8u() {
    LOG_SCOPE_F(INFO, "Read plain image (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint8_t>();
}

Image16u PlainReader::read16u() {
    LOG_SCOPE_F(INFO, "Read plain image (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    return read<uint16_t>();
}

Imagef PlainReader::readf() {
    LOG_SCOPE_F(INFO, "Read plain image (float)");
    LOG_S(INFO) << "Path: " << path();

    return read<float>();
}

template <typename T>
Image<T> PlainReader::read() {
    validateType<T>();

    Image<T> image(layoutDescriptor());

    mStream->seekg(0, std::istream::end);
    int64_t fileSize = mStream->tellg();
    mStream->seekg(0);

    if (static_cast<uint64_t>(fileSize) != image.size() * sizeof(T)) {
        throw IOError(MODULE,
                      "File size does not match expected buffer size (expected " +
                              std::to_string(image.size() * sizeof(T)) + ", got " + std::to_string(fileSize) + ")");
    }

    mStream->read(reinterpret_cast<char *>(image.data()), image.size());

    return image;
}

void PlainWriter::write(const Image8u &image) const {
    LOG_SCOPE_F(INFO, "Write plain image (8 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint8_t>(image);
}

void PlainWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write plain image (16 bits)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<uint16_t>(image);
}

void PlainWriter::write(const Imagef &image) const {
    LOG_SCOPE_F(INFO, "Write plain image (float)");
    LOG_S(INFO) << "Path: " << path();

    writeImpl<float>(image);
}

template <typename T>
void PlainWriter::writeImpl(const Image<T> &image) const {
    std::ofstream stream(path(), std::ios::binary);
    if (!stream) {
        throw IOError("Cannot open file for writing: " + path());
    }

    stream.write(reinterpret_cast<const char *>(image.data()), image.size() * sizeof(T));
}

} // namespace cxximg
