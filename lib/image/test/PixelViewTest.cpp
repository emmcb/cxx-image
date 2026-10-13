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

#include "cxximg/image/Image.h"
#include "cxximg/image/view/PixelView.h"
#include "cxximg/math/Matrix.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <type_traits>

#include <gtest/gtest.h>

using namespace cxximg;

/// Checks whole-pixel volume expressions, including matrix transforms, without writing borders or padding.
TEST(PixelViewTest, VolumeExpressions) {
    for (const auto layoutKind : {ImageLayout::PLANAR, ImageLayout::INTERLEAVED}) {
        SCOPED_TRACE(toString(layoutKind));
        const auto layout = LayoutDescriptor::Builder(2, 2, 3)
                                    .numChannels(3)
                                    .imageLayout(layoutKind)
                                    .border(1)
                                    .widthAlignment(8)
                                    .heightAlignment(4)
                                    .depthAlignment(4)
                                    .build();
        Image32i image(layout);
        std::fill_n(image.data(), image.size(), -99);
        PixelView<int32_t, 3> pixels(image.descriptor());
        pixels = [](int x, int y, int z) {
            const int value = 100 * z + 10 * y + x;
            return Pixel<int32_t, 3>{value, value + 1, value + 2};
        };
        pixels += Pixel<int32_t, 3>{1, 2, 3};
        pixels -= Pixel<int32_t, 3>{1, 1, 1};
        pixels *= Pixel<int32_t, 3>(2);
        pixels /= Pixel<int32_t, 3>(2);
        pixels = Matrix<3, 3>::diag(Pixel<int32_t, 3>{1, 2, 3}) * pixels;

        const auto lastPixel = pixels(1, 1, 2);
        EXPECT_EQ(lastPixel[0], 211);
        EXPECT_EQ(lastPixel[1], 426);
        EXPECT_EQ(lastPixel[2], 645);
        EXPECT_EQ(pixels(1, 1)[0], 11);
        EXPECT_EQ(pixels.depth(), 3);
        EXPECT_EQ(std::count_if(image.data(), image.data() + image.size(), [](int32_t value) { return value >= 0; }),
                  36);

        int visits = 0;
        pixels.forEach([&](int x, int y, int z) {
            EXPECT_LT(x, 2);
            EXPECT_LT(y, 2);
            EXPECT_LT(z, 3);
            ++visits;
        });
        EXPECT_EQ(visits, 12);
    }
}

/// Checks depth broadcasting and whole-pixel evaluation of a pixel view.
TEST(PixelViewTest, BroadcastAndEvaluate) {
    const Image32i flatImage(LayoutDescriptor::Builder(2, 2).numChannels(2).build(), 9);
    const PixelView<int32_t, 2> flat(flatImage.descriptor());
    Image32i volume(LayoutDescriptor::Builder(2, 2, 3).numChannels(2).build(), 0);
    PixelView<int32_t, 2> pixels(volume.descriptor());
    pixels = flat + Pixel<int32_t, 2>{1, 2};
    pixels += expr::shift(flat, 0, 0);
    pixels -= Pixel<int32_t, 2>(1);

    const auto pixel = expr::evaluate(pixels, 1, 1, 2);
    EXPECT_EQ(pixel[0], 18);
    EXPECT_EQ(pixel[1], 19);
    EXPECT_EQ(expr::evaluate(flat, 1, 1, 99)[0], 9);
    static_assert(!std::is_reference_v<decltype(expr::evaluate(pixels, 1, 1, 2))>);

    const Image32i singleImage(LayoutDescriptor::Builder(2, 2).numChannels(1).build(), 7);
    const PixelView<int32_t, 1> single(singleImage.descriptor());
    EXPECT_EQ(expr::evaluate(single, 1, 1, 99)[0], 7);
}

/// Checks writable pixel ROI views preserve depth and source slice strides.
TEST(PixelViewTest, VolumeRoi) {
    Image32i image(LayoutDescriptor::Builder(3, 3, 2).numChannels(2).build(), 0);
    PixelView<int32_t, 2> pixels(image.descriptor());
    auto roi = pixels[Rect{1, 1, 1, 1}];
    roi = [](int, int, int z) { return Pixel<int32_t, 2>{z + 1, z + 2}; };

    EXPECT_EQ(roi.width(), 1);
    EXPECT_EQ(roi.height(), 1);
    EXPECT_EQ(roi.depth(), 2);
    EXPECT_EQ(image(1, 1, 1, 0), 2);
    EXPECT_EQ(image(1, 1, 1, 1), 3);
    EXPECT_EQ(image(0, 0, 1, 0), 0);

    auto other = pixels[Rect{0, 0, 1, 1}];
    other = Pixel<int32_t, 2>{7, 8};
    roi = other;
    EXPECT_EQ(image(1, 1, 1, 0), 7);
    EXPECT_EQ(image(1, 1, 1, 1), 8);
    EXPECT_EQ(image(2, 2, 1, 0), 0);
}

/// Rejects subsampled channels while allowing a view of the full-resolution luma channel.
TEST(PixelViewTest, RejectsSubsampledChannels) {
    std::array<int32_t, 64> data;
    std::iota(data.begin(), data.end(), 0);
    const Image32i image(LayoutDescriptor::Builder(4, 4, 3).imageLayout(ImageLayout::NV12).build(), data.data());
    EXPECT_DEBUG_DEATH((PixelView<int32_t, 3>(image.descriptor())), "subsample");

    const PixelView<int32_t, 1> luma(image.descriptor());
    EXPECT_EQ(luma(0, 0, 2)[0], 32);
    EXPECT_EQ(expr::evaluate(luma, 0, 0, 2)[0], 32);
}
