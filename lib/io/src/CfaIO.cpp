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

#include "CfaIO.h"

#include <loguru.hpp>

using namespace std::string_literals;

namespace cxximg {

static const std::string MODULE = "CFA";

static PixelType phaseToPixelType(uint8_t phase) {
    switch (phase) {
        case 0:
            return PixelType::BAYER_GBRG;
        case 1:
            return PixelType::BAYER_BGGR;
        case 2:
            return PixelType::BAYER_RGGB;
        case 3:
            return PixelType::BAYER_GRBG;
        default:
            throw IOError(MODULE, "Unsupported bayer phase: " + std::to_string(phase));
    }
}

static uint8_t pixelTypeToPhase(PixelType pixelType) {
    switch (pixelType) {
        case PixelType::BAYER_GBRG:
        case PixelType::QUADBAYER_GBRG:
            return 0;
        case PixelType::BAYER_BGGR:
        case PixelType::QUADBAYER_BGGR:
            return 1;
        case PixelType::BAYER_RGGB:
        case PixelType::QUADBAYER_RGGB:
            return 2;
        case PixelType::BAYER_GRBG:
        case PixelType::QUADBAYER_GRBG:
            return 3;
        default:
            throw IOError(MODULE, "Unsupported pixel type: "s + toString(pixelType));
    }
}

void CfaReader::initialize() {
    CfaHeader header = {};
    mStream->read(reinterpret_cast<char *>(&header), sizeof(header));

    if (mStream->fail()) {
        throw IOError(MODULE, "Failed to read header");
    }

    const int width = 2 * static_cast<int>(header.uCFABlockWidth);
    const int height = 2 * static_cast<int>(header.uCFABlockHeight);

    mDescriptor = {LayoutDescriptor::Builder(width, height)
                           .pixelType(phaseToPixelType(header.phase))
                           .pixelPrecision(header.precision)
                           .build(),
                   PixelRepresentation::UINT16};
}

Image16u CfaReader::read16u() {
    LOG_SCOPE_F(INFO, "Read CFA");
    LOG_S(INFO) << "Path: " << path();

    Image16u image(layoutDescriptor());

    int64_t curPos = mStream->tellg();
    mStream->seekg(0, std::istream::end);
    int64_t endPos = mStream->tellg();

    if (static_cast<uint64_t>(endPos - curPos) != image.size() * sizeof(uint16_t)) {
        throw IOError(MODULE,
                      "File size does not match expected buffer size (expected " +
                              std::to_string(image.size() * sizeof(uint16_t)) + ", got " +
                              std::to_string(endPos - curPos) + ")");
    }

    mStream->seekg(curPos);
    mStream->read(reinterpret_cast<char *>(image.data()), image.size() * sizeof(uint16_t));

    return image;
}

void CfaWriter::write(const Image16u &image) const {
    LOG_SCOPE_F(INFO, "Write CFA");
    LOG_S(INFO) << "Path: " << path();

    CfaHeader header = {.cfaID = 1128677664, // CFA magic number
                        .version = 1,
                        .uCFABlockWidth = static_cast<uint32_t>(image.width() / 2),
                        .uCFABlockHeight = static_cast<uint32_t>(image.height() / 2),
                        .phase = pixelTypeToPhase(image.pixelType()),
                        .precision = static_cast<uint8_t>(image.pixelPrecision() > 0 ? image.pixelPrecision() : 16),
                        .padding = {0}};

    mStream->write(reinterpret_cast<const char *>(&header), sizeof(header));
    mStream->write(reinterpret_cast<const char *>(image.data()), image.size() * sizeof(uint16_t));
}

} // namespace cxximg
