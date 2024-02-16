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

#include "model/ImageMetadata.h"

#include <optional>
#include <string>

namespace cxximg {

/// @defgroup parser Image metadata parser
/// @copydoc libparser

/// Metadata parser functions
/// @ingroup parser
namespace parser {

/// Try to read the image metadata from different sources.
///
/// This method will successively check:
///   1. If metadataPath is given and is a json file, then read it directly.
///   2. If metadataPath is given and is a directory, then try to read a json file in this directory with the same name
///      than the image.
///   3. If metadataPath is not given, try to read a json file in the image directory with the same name than the image.
///   4. If everything has failed then return an empty optional.
std::optional<ImageMetadata> readMetadata(const std::string& imagePath, const std::optional<std::string>& metadataPath);

/// Read metadata from the given file.
ImageMetadata readMetadata(const std::string& metadataPath);

/// Write metadata to the given file.
void writeMetadata(const ImageMetadata& metadata, const std::string& metadataPath);

} // namespace parser

} // namespace cxximg
