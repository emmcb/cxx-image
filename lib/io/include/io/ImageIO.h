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

#include <istream>
#include <memory>
#include <string>

namespace cxximg {

/// @defgroup io Image IO library
/// @copydoc libio

/// Image IO functions
/// @ingroup io
namespace io {

/// Allocates a new ImageReader with the ability to read the given file.
std::unique_ptr<ImageReader> makeReader(const std::string &path, const ImageReader::Options &options = {});

/// Allocates a new ImageReader with the ability to read the given stream.
std::unique_ptr<ImageReader> makeReader(std::istream *stream, const ImageReader::Options &options = {});

/// Allocates a new ImageReader with the ability to read the given stream, with path as a file format hint.
std::unique_ptr<ImageReader> makeReader(const std::string &path,
                                        std::istream *stream,
                                        const ImageReader::Options &options = {});

/// Allocates a new ImageWriter with the ability to write the given file.
std::unique_ptr<ImageWriter> makeWriter(const std::string &path, const ImageWriter::Options &options = {});

} // namespace io

} // namespace cxximg
