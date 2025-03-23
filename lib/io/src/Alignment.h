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

#include "cxximg/image/LayoutDescriptor.h"

#include <optional>

namespace cxximg {

namespace detail {

/// Guesses the pixel size that matches the given file size.
inline int guessPixelSize(const LayoutDescriptor::Builder &builder, const int64_t fileSize) {
    LayoutDescriptor::Builder localBuilder(builder);

    const int64_t refBufferSize = localBuilder.widthAlignment(1).build().requiredBufferSize();

    int pixelSize = 2;
    for (; refBufferSize * pixelSize <= fileSize; pixelSize *= 2) {
    }

    return pixelSize / 2;
}

/// Guesses the width alignment, if it exists, that matches the given file size.
inline std::optional<int> guessWidthAlignment(const LayoutDescriptor::Builder &builder, const int64_t fileSize) {
    LayoutDescriptor::Builder localBuilder(builder);

    const int pixelSize = guessPixelSize(builder, fileSize);
    int64_t estimatedFileSize = 0;
    int widthAlignment = 1;

    while (estimatedFileSize < fileSize) {
        const LayoutDescriptor descriptor = localBuilder.widthAlignment(widthAlignment).build();
        estimatedFileSize = descriptor.requiredBufferSize() * pixelSize;
        if (estimatedFileSize == fileSize) {
            return widthAlignment;
        }
        widthAlignment *= 2;
    }

    return std::nullopt;
}

} // namespace detail

} // namespace cxximg
