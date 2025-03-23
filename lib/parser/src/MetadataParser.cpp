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

#include "MetadataDto.h"

#include "cxximg/parser/Exceptions.h"
#include "cxximg/parser/MetadataParser.h"

#include <loguru.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace cxximg {

struct CurrentPathRestorer final {
    fs::path currentPath = fs::current_path();
    ~CurrentPathRestorer() { fs::current_path(currentPath); }
};

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

    if (!fs::exists(metadataGuess)) {
        LOG_S(INFO) << "No metadata found at " << metadataGuess;
        return std::nullopt;
    }

    LOG_S(INFO) << "Using metadata: " << metadataGuess;

    return readMetadata(metadataGuess);
}

ImageMetadata readMetadata(const std::string& metadataPath) {
    std::ifstream ifs(metadataPath);
    if (!ifs) {
        throw ParserError("Cannot open file for reading: " + metadataPath);
    }

    CurrentPathRestorer currentPathRestorer{};
    fs::path fsMetadataPath(metadataPath);
    if (fsMetadataPath.has_parent_path()) {
        fs::current_path(fsMetadataPath.parent_path());
    }

    ImageMetadata metadata;
    try {
        json_dto::from_stream(ifs, metadata);
    } catch (const json_dto::ex_t& e) {
        throw ParserError(e.what());
    }

    return metadata;
}

void writeMetadata(const ImageMetadata& metadata, const std::string& metadataPath) {
    std::ofstream ofs(metadataPath);
    if (!ofs) {
        throw ParserError("Cannot open file for writing: " + metadataPath);
    }

    CurrentPathRestorer currentPathRestorer{};
    fs::path fsMetadataPath(metadataPath);
    if (fsMetadataPath.has_parent_path()) {
        fs::current_path(fsMetadataPath.parent_path());
    }

    try {
        json_dto::to_stream(ofs, metadata, json_dto::pretty_writer_params_t{});
    } catch (const json_dto::ex_t& e) {
        throw ParserError(e.what());
    }
}

} // namespace parser

} // namespace cxximg
