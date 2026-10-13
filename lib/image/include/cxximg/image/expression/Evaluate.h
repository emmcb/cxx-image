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

#include "cxximg/util/compiler.h"

#include <cassert>
#include <type_traits>

namespace cxximg {

template <int M, int N>
class Matrix;

template <typename T, int N>
class Pixel;

template <typename T>
class Image;

template <typename T>
class ImageView;

template <typename T>
class ChannelView;

template <typename T, int N>
class PixelView;

namespace expr {

/// Preserves a matrix as a constant rather than invoking its row and column accessor.
template <int M, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Matrix<M, N>& matrix,
                                                  [[maybe_unused]] int x,
                                                  [[maybe_unused]] int y) noexcept {
    return matrix;
}

/// Evaluates a channel slice, broadcasting a singleton depth dimension.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const ChannelView<T>& channelView, int x, int y, int z) noexcept {
    return channelView(x, y, channelView.depth() > 1 ? z : 0);
}

/// Evaluates a complete pixel view slice, broadcasting a singleton depth dimension.
template <typename T, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const PixelView<T, N>& pixelView, int x, int y, int z) noexcept {
    return pixelView(x, y, pixelView.depth() > 1 ? z : 0);
}

/// Evaluates XY coordinates through a const callable, or borrows the constant unchanged.
/// Callables must not throw; borrowed constants must outlive references to the result.
template <typename Expr>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Expr& expr, int x, int y) noexcept {
    if constexpr (std::is_invocable_v<const Expr&, int, int>) {
        return expr(x, y);
    } else {
        return expr;
    }
}

/// Evaluates XYZ coordinates, broadcasting XY callables and constants across depth.
template <typename Expr>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Expr& expr, int x, int y, int z) noexcept {
    if constexpr (std::is_invocable_v<const Expr&, int, int, int>) {
        return expr(x, y, z);
    } else {
        return evaluate(expr, x, y);
    }
}

/// Evaluates XYZC coordinates, broadcasting callables and constants that omit dimensions.
template <typename Expr>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Expr& expr, int x, int y, int z, int c) noexcept {
    if constexpr (std::is_invocable_v<const Expr&, int, int, int, int>) {
        return expr(x, y, z, c);
    } else {
        return evaluate(expr, x, y, z);
    }
}

/// Evaluates an image view channel, broadcasting singleton depth and channel dimensions.
template <typename T>
UTIL_ALWAYS_INLINE inline T evaluate(const ImageView<T>& imageView, int x, int y, int z, int c) noexcept {
    const int channel = imageView.numChannels() > 1 ? c : 0;
    assert(channel >= 0 && channel < imageView.numChannels());

    const auto& channelDescriptor = imageView.layoutDescriptor().channels[channel];
    const int slice = imageView.depth() > (1 << channelDescriptor.subsample) ? z : 0;
    return imageView(x, y, slice, channel);
}

/// Evaluates a single-channel image view at XYZ, using its first slice when depth is omitted.
template <typename T>
UTIL_ALWAYS_INLINE inline T evaluate(const ImageView<T>& imageView, int x, int y, int z = 0) noexcept {
    assert(imageView.numChannels() == 1);
    return evaluate(imageView, x, y, z, 0);
}

/// Evaluates an owning image's spatial coordinates through its view adapter.
template <typename T>
UTIL_ALWAYS_INLINE inline T evaluate(const Image<T>& image, int x, int y, int z = 0) noexcept {
    return evaluate(static_cast<const ImageView<T>&>(image), x, y, z);
}

/// Evaluates an owning image channel through its view adapter.
template <typename T>
UTIL_ALWAYS_INLINE inline T evaluate(const Image<T>& image, int x, int y, int z, int c) noexcept {
    return evaluate(static_cast<const ImageView<T>&>(image), x, y, z, c);
}

/// Selects a constant pixel's channel by value, broadcasting a singleton channel across the image.
template <typename T, int N>
UTIL_ALWAYS_INLINE inline T evaluate(const Pixel<T, N>& pixel,
                                     [[maybe_unused]] int x,
                                     [[maybe_unused]] int y,
                                     [[maybe_unused]] int z,
                                     int c) noexcept {
    const int channel = N > 1 ? c : 0;
    assert(channel >= 0 && channel < N);
    return pixel[channel];
}

} // namespace expr

} // namespace cxximg
