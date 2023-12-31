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

#include "parser/MetadataParser.h"
#include "MetadataDto.h"
#include "parser/Exceptions.h"

#include <loguru.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace cxximg {

namespace parser {

std::optional<ImageMetadata> readMetadata(const std::string& imagePath,
                                          const std::optional<std::string>& metadataPath) {
    const std::string metadataGuess =
            [&]() {
                static constexpr char METADATA_EXTENSION[] = ".json";

                // We have a medatadata path
                if (metadataPath) {
                    fs::path fsMetadata(*metadataPath);

                    // If the metadata path is a directory, we assume that it will contain a metadata file with the same
                    // name than the image.
                    if (fs::is_directory(fsMetadata)) {
                        return fsMetadata / fs::path(imagePath).filename().replace_extension(METADATA_EXTENSION);
                    }

                    return fsMetadata;
                }

                // We do not have a metadata path, try with a sidecar along the image
                return fs::path(imagePath).replace_extension(METADATA_EXTENSION);
            }()
                    .string();

    LOG_S(INFO) << "Using metadata: " << metadataGuess;

    if (!fs::exists(metadataGuess)) {
        LOG_S(INFO) << "No metadata found";
        return std::nullopt;
    }

    return readMetadata(metadataGuess);
}

ImageMetadata readMetadata(const std::string& metadataPath) {
    std::ifstream ifs(metadataPath);
    if (!ifs) {
        throw ParserError("Cannot open input file for reading");
    }

    ImageMetadata metadata;

    try {
        json_dto::from_stream(ifs, metadata);
    } catch (const json_dto::ex_t& e) {
        throw ParserError(e.what());
    }

    return metadata;
}

void writeMetadata(const ImageMetadata& metadata, const std::string& outputPath) {
    std::ofstream ofs(outputPath);
    if (!ofs) {
        throw ParserError("Cannot open output file for writing");
    }

    try {
        json_dto::to_stream(ofs, metadata, json_dto::pretty_writer_params_t{});
    } catch (const json_dto::ex_t& e) {
        throw ParserError(e.what());
    }
}

} // namespace parser

} // namespace cxximg
