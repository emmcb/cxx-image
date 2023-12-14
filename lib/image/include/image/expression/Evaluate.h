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

#include "util/compiler.h"

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
class PlaneView;

namespace expr {

/// If expression is invocable with the provided coordinates, then invoke it.
template <typename Expr, typename... Coord, std::enable_if_t<std::is_invocable_v<Expr, Coord...>, bool> = true>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Expr &expr, Coord... coords) noexcept {
    return expr(coords...);
}

/// If expression is not invocable with the provided coordinates, then return it as-is.
template <typename Expr, typename... Coord, std::enable_if_t<!std::is_invocable_v<Expr, Coord...>, bool> = true>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Expr &expr, [[maybe_unused]] Coord... coords) noexcept {
    return expr;
}

/// When omitting the plane number, invoke the image at plane 0.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Image<T> &image, int x, int y) noexcept {
    assert(image.numPlanes() == 1);
    return image(x, y, 0);
}

/// Ensure we always invoke a one plane image at plane 0.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Image<T> &image, int x, int y, int n) noexcept {
    return image(x, y, image.numPlanes() > 1 ? n : 0);
}

/// When omitting the plane number, invoke the image at plane 0.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const ImageView<T> &imageView, int x, int y) noexcept {
    assert(imageView.numPlanes() == 1);
    return imageView(x, y, 0);
}

/// Ensure we always invoke a one plane image at plane 0.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const ImageView<T> &imageView, int x, int y, int n) noexcept {
    return imageView(x, y, imageView.numPlanes() > 1 ? n : 0);
}

/// Invoke a plane at (x, y) coordinates whatever the plane number n.
template <typename T>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const PlaneView<T> &planeView,
                                                  int x,
                                                  int y,
                                                  [[maybe_unused]] int n) noexcept {
    return planeView(x, y);
}

/// Subset the pixel at plane number n.
template <typename T, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Pixel<T, N> &pixel,
                                                  [[maybe_unused]] int x,
                                                  [[maybe_unused]] int y,
                                                  int n) noexcept {
    return pixel[n];
}

/// Do not attempt to invoke a matrix with (x, y) coordinates.
template <int M, int N>
UTIL_ALWAYS_INLINE inline decltype(auto) evaluate(const Matrix<M, N> &matrix,
                                                  [[maybe_unused]] int x,
                                                  [[maybe_unused]] int y) noexcept {
    return matrix;
}

} // namespace expr

} // namespace cxximg
