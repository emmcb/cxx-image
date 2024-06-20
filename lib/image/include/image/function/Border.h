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

#include "image/Image.h"

namespace cxximg {

namespace image {

/// How image borders are handled.
enum class BorderMode {
    CONSTANT, ///< 000|abc|000
    MIRROR,   ///< cb|abc|ba
    NEAREST,  ///< aaa|abc|ccc
    REFLECT   ///< cba|abc|cba
};

/// Updates the image border values.
/// The borders must already have been allocated with the makeBorders() function.
template <BorderMode MODE, typename T>
void updateBorders(const ImageView<T> &img, int borderSize) {
    Roi leftRoi = {-borderSize, 0, borderSize, img.height()};
    Roi rightRoi = {img.width(), 0, borderSize, img.height()};
    Roi topRoi = {0, -borderSize, img.width(), borderSize};
    Roi bottomRoi = {0, img.height(), img.width(), borderSize};

    Roi topLeftRoi = {-borderSize, -borderSize, borderSize, borderSize};
    Roi topRightRoi = {img.width(), -borderSize, borderSize, borderSize};
    Roi bottomLeftRoi = {-borderSize, img.height(), borderSize, borderSize};
    Roi bottomRightRoi = {img.width(), img.height(), borderSize, borderSize};

    if constexpr (MODE == BorderMode::CONSTANT) {
        img[leftRoi] = 0;
        img[rightRoi] = 0;
        img[topRoi] = 0;
        img[bottomRoi] = 0;

        img[topLeftRoi] = 0;
        img[topRightRoi] = 0;
        img[bottomLeftRoi] = 0;
        img[bottomRightRoi] = 0;
    }

    if constexpr (MODE == BorderMode::MIRROR) {
        img[leftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x, y, coords...);
        };

        img[rightRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 2, y, coords...);
        };

        img[topRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, x, borderSize - y, coords...);
        };

        img[bottomRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, x, img.height() - y - 2, coords...);
        };

        img[topLeftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x, borderSize - y, coords...);
        };

        img[topRightRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 2, borderSize - y, coords...);
        };

        img[bottomLeftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x, img.height() - y - 2, coords...);
        };

        img[bottomRightRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 2, img.height() - y - 2, coords...);
        };
    }

    if constexpr (MODE == BorderMode::NEAREST) {
        img[leftRoi] = [&img]([[maybe_unused]] int x, int y, auto... coords) {
            return expr::evaluate(img, 0, y, coords...);
        };

        img[rightRoi] = [&img]([[maybe_unused]] int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - 1, y, coords...);
        };

        img[topRoi] = [&img](int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, x, 0, coords...);
        };

        img[bottomRoi] = [&img](int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, x, img.height() - 1, coords...);
        };

        img[topLeftRoi] = [&img]([[maybe_unused]] int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, 0, 0, coords...);
        };

        img[topRightRoi] = [&img]([[maybe_unused]] int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, img.width() - 1, 0, coords...);
        };

        img[bottomLeftRoi] = [&img]([[maybe_unused]] int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, 0, img.height() - 1, coords...);
        };

        img[bottomRightRoi] = [&img]([[maybe_unused]] int x, [[maybe_unused]] int y, auto... coords) {
            return expr::evaluate(img, img.width() - 1, img.height() - 1, coords...);
        };
    }

    if constexpr (MODE == BorderMode::REFLECT) {
        img[leftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x - 1, y, coords...);
        };

        img[rightRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 1, y, coords...);
        };

        img[topRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, x, borderSize - y - 1, coords...);
        };

        img[bottomRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, x, img.height() - y - 1, coords...);
        };

        img[topLeftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x - 1, borderSize - y - 1, coords...);
        };

        img[topRightRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 1, borderSize - y - 1, coords...);
        };

        img[bottomLeftRoi] = [&img, borderSize](int x, int y, auto... coords) {
            return expr::evaluate(img, borderSize - x - 1, img.height() - y - 1, coords...);
        };

        img[bottomRightRoi] = [&img](int x, int y, auto... coords) {
            return expr::evaluate(img, img.width() - x - 1, img.height() - y - 1, coords...);
        };
    }
}

/// Allocates a new image from an existing one, with borders initialized using the given border mode.
template <BorderMode MODE, typename T>
Image<T> makeBorders(const ImageView<T> &img, int borderSize) {
    Image<T> copy(LayoutDescriptor::Builder(img.layoutDescriptor()).border(borderSize).build(), img);
    updateBorders<MODE>(copy, borderSize);

    return copy;
}

} // namespace image

} // namespace cxximg
