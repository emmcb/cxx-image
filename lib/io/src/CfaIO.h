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

#include "io/ImageReader.h"
#include "io/ImageWriter.h"

#include "util/File.h"

namespace cxximg {

// See DxO Analyzer documentation, page 67
struct CfaHeader final {
    uint32_t cfaID;
    uint32_t version;
    uint32_t uCFABlockWidth;
    uint32_t uCFABlockHeight;
    uint8_t phase;
    uint8_t precision;
    uint8_t padding[110];
};

static_assert(sizeof(CfaHeader) == 128, "CfaHeader must by 128 bytes");

class CfaReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        std::vector<uint8_t> header = file::readBinary(path, 4);
        return header[0] == ' ' && header[1] == 'A' && header[2] == 'F' && header[3] == 'C';
    }

    CfaReader(const std::string &path, const Options &options);

    Image16u read16u() override;
};

class CfaWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) { return file::extension(path) == "cfa"; }

    CfaWriter(const std::string &path, const Options &options) : ImageWriter(path, options) {}

    bool acceptDescriptor(const LayoutDescriptor &descriptor) const override {
        return image::isBayerPixelType(descriptor.pixelType) || image::isQuadBayerPixelType(descriptor.pixelType);
    }

    void write(const Image16u &image) const override;
};

} // namespace cxximg
