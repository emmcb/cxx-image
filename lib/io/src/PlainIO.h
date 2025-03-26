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

#pragma once

#include "cxximg/io/ImageReader.h"
#include "cxximg/io/ImageWriter.h"

#include "cxximg/util/File.h"

namespace cxximg {

class PlainReader final : public ImageReader {
public:
    static bool accept(const std::string &path) {
        std::string extension = file::extension(path);
        return (extension == "nv12" || extension == "y8" || extension == "plain16");
    }

    using ImageReader::ImageReader;

    void initialize() override;

    Image8u read8u() override;
    Image16u read16u() override;
    Imagef readf() override;

private:
    template <typename T>
    Image<T> read();
};

class PlainWriter final : public ImageWriter {
public:
    static bool accept(const std::string &path) {
        std::string extension = file::extension(path);
        return (extension == "nv12" || extension == "y8" || extension == "plain16");
    }

    using ImageWriter::ImageWriter;

    bool acceptDescriptor([[maybe_unused]] const LayoutDescriptor &descriptor) const override { return true; }

    void write(const Image8u &image) const override;
    void write(const Image16u &image) const override;
    void write(const Imagef &image) const override;

private:
    template <typename T>
    void writeImpl(const Image<T> &image) const;
};

} // namespace cxximg
