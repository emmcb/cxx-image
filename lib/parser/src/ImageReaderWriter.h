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

#include "JsonDto.h"

#include "io/ImageIO.h"

namespace cxximg {

struct ImageLoader final {
    static void read(DynamicMatrix& matrix, const rapidjson::Value& object) {
        std::string path;
        json_dto::read_json_value(path, object);

        Imagef image = [&]() {
            using namespace std::string_literals;

            const auto reader = io::makeReader(path);
            switch (reader->pixelRepresentation()) {
                case PixelRepresentation::FLOAT:
                    return image::convertLayout(reader->readf(), ImageLayout::PLANAR);
                case PixelRepresentation::UINT8:
                    return image::convertPixelPrecision<float>(reader->read8u(), ImageLayout::PLANAR);
                case PixelRepresentation::UINT16:
                    return image::convertPixelPrecision<float>(reader->read16u(), ImageLayout::PLANAR);
                default:
                    throw json_dto::ex_t("Unsupported pixel representation "s +
                                         toString(reader->pixelRepresentation()));
            }
        }();

        matrix = DynamicMatrix(image.height(), image.width(), image.data());
    }

    static void write([[maybe_unused]] const DynamicMatrix& matrix,
                      [[maybe_unused]] rapidjson::Value& object,
                      [[maybe_unused]] rapidjson::MemoryPoolAllocator<>& allocator) {
        // nothing to do
    }
};

} // namespace cxximg
