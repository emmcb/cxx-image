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
#include "cxximg/image/Interpolator.h"
#include "cxximg/image/view/PixelView.h"
#include "cxximg/math/Matrix.h"

#include <array>
#include <numeric>
#include <type_traits>

#include <gtest/gtest.h>

using namespace cxximg;

/// Checks slice addressing and nested expression evaluation in a padded planar volume.
TEST(EvaluateTest, PlanarVolume) {
    std::array<int32_t, 128> data;
    std::iota(data.begin(), data.end(), 0);
    const auto layout = LayoutDescriptor::Builder(2, 2, 3)
                                .numChannels(2)
                                .widthAlignment(4)
                                .heightAlignment(4)
                                .depthAlignment(4)
                                .build();
    const Image32i image(layout, data.data());
    const ImageView32i view(image.descriptor());

    EXPECT_EQ(expr::evaluate(image, 1, 1, 2, 1), 101);
    EXPECT_EQ(expr::evaluate(view, 1, 1, 2, 1), 101);
    EXPECT_EQ(expr::evaluate(image.channel(1), 1, 1, 2, 99), 101);
    EXPECT_EQ(expr::evaluate(image.channel(1), 1, 1, 1), 85);
    EXPECT_EQ(image(1, 1, 1), 69); // Direct three-coordinate image access retains its channel argument.

    const auto expression = expr::cast<int32_t>(expr::shift(image * 2 + Pixel<int32_t, 2>{1, 2}, 1, 0));
    EXPECT_EQ(expr::evaluate(expression, 0, 1, 2, 1), 204);
    EXPECT_EQ(expr::evaluate(expr::resize(image, 2, 2, false), 1, 1, 2, 1), 101);
}

/// Checks interleaved slices, writable access, and converting a volume channel to an image view.
TEST(EvaluateTest, InterleavedVolume) {
    std::array<int32_t, 128> data;
    std::iota(data.begin(), data.end(), 0);
    const auto layout = LayoutDescriptor::Builder(2, 2, 3)
                                .numChannels(2)
                                .imageLayout(ImageLayout::INTERLEAVED)
                                .widthAlignment(8)
                                .heightAlignment(4)
                                .depthAlignment(4)
                                .build();
    Image32i image(layout, data.data());
    auto channel = image.channel(1);
    const ImageView32i channelImage(channel);

    EXPECT_EQ(expr::evaluate(image, 1, 1, 2, 1), 75);
    EXPECT_EQ(expr::evaluate(channel, 1, 1, 2, 0), 75);
    EXPECT_EQ(channelImage.depth(), 3);
    EXPECT_EQ(expr::evaluate(channelImage, 1, 1, 2, 99), 75);

    image(1, 1, 2, 1) = 123;
    EXPECT_EQ(image[75], 123);
    channel(0, 0, 1) = 456;
    EXPECT_EQ(image[33], 456);
}

/// Checks independent broadcasting of singleton depth and channel dimensions.
TEST(EvaluateTest, SingletonDimensions) {
    const std::array<int32_t, 8> data = {10, 11, 12, 13, 14, 15, 16, 17};
    const Image32i flat(LayoutDescriptor::Builder(2, 2).numChannels(1).build(), data.data());
    EXPECT_EQ(expr::evaluate(flat, 1, 1, 9, 8), 13);
    EXPECT_EQ(expr::evaluate(ImageView32i(flat.descriptor()), 1, 1, 9, 8), 13);
    EXPECT_EQ(expr::evaluate(flat.channel(0), 1, 1, 9, 8), 13);

    const Image32i volume(LayoutDescriptor::Builder(2, 2, 2).numChannels(1).build(), data.data());
    EXPECT_EQ(expr::evaluate(volume, 1, 1, 1, 8), 17);
    EXPECT_EQ(expr::evaluate(volume, 1, 1, 1), 17);
    EXPECT_EQ(expr::evaluate(ImageView32i(volume.descriptor()), 1, 1, 1), 17);
    EXPECT_EQ(expr::evaluate(volume * 2 + flat, 1, 1, 1, 8), 47);

    const Image32i multiChannel(LayoutDescriptor::Builder(2, 2).numChannels(2).build(), data.data());
    EXPECT_EQ(expr::evaluate(multiChannel, 1, 1, 9, 1), 17);
}

/// Checks callable coordinate forwarding and constants that must not be invoked as spatial functions.
TEST(EvaluateTest, CallablesAndConstants) {
    const auto volume = [](int x, int y, int z, int c) { return x + 10 * y + 100 * z + 1000 * c; };
    const auto spatialVolume = [](int x, int y, int z) { return x + 10 * y + 100 * z; };
    const auto spatial = [](int x, int y) { return x + 10 * y; };
    EXPECT_EQ(expr::evaluate(volume, 1, 2, 3, 4), 4321);
    EXPECT_EQ(expr::evaluate(spatialVolume, 1, 2, 3, 4), 321);
    EXPECT_EQ(expr::evaluate(spatialVolume, 1, 2, 3), 321);
    EXPECT_EQ(expr::evaluate(spatial, 1, 2, 3, 4), 21);
    EXPECT_EQ(expr::evaluate(7, 1, 2, 3, 4), 7);

    const Pixel<int32_t, 2> pixel{7, 11};
    EXPECT_EQ(expr::evaluate(pixel, 99, 88, 77, 1), 11);
    EXPECT_EQ(&expr::evaluate(pixel, 99, 88, 77), &pixel);

    const Matrix<2, 2> matrix{{1, 2}, {3, 4}};
    EXPECT_EQ(&expr::evaluate(matrix, 99, 88, 77, 66), &matrix);
    EXPECT_EQ(&expr::evaluate(matrix, 99, 88, 77), &matrix);
}

/// Checks channel-local slice coordinates and broadcasting after depth subsampling.
TEST(EvaluateTest, SubsampledVolume) {
    std::array<int32_t, 36> data;
    std::iota(data.begin(), data.end(), 0);
    const auto layout = LayoutDescriptor::Builder(3, 3, 2)
                                .numChannels(2)
                                .imageLayout(ImageLayout::CUSTOM)
                                .channelSubsample(1, 1)
                                .build();
    const ImageView32i image(layout, data.data());
    EXPECT_EQ(image.channel(1).depth(), 1);
    EXPECT_EQ(expr::evaluate(image, 1, 1, 0, 1), 35);
    EXPECT_EQ(expr::evaluate(image, 1, 1, 99, 1), 35);
    EXPECT_EQ(expr::evaluate(image.channel(1), 1, 1, 99, 0), 35);
}

/// Checks that converted coordinates retain slice semantics and all specialized broadcasting rules.
TEST(EvaluateTest, CoordinateTypes) {
    const std::array<int32_t, 8> data = {10, 11, 12, 13, 14, 15, 16, 17};
    const Image32i image(LayoutDescriptor::Builder(2, 2, 2).numChannels(1).build(), data.data());
    const ImageView32i view(image.descriptor());
    const PixelView<int32_t, 1> pixels(image.descriptor());
    EXPECT_EQ(expr::evaluate(image, 1, 1), 13);
    EXPECT_EQ(expr::evaluate(view, 1L, 1L), 13);
    EXPECT_EQ(expr::evaluate(image, 1L, 1L, 1L), 17);
    EXPECT_EQ(expr::evaluate(view, 1L, 1L, 1), 17);
    EXPECT_EQ(expr::evaluate(view, 1u, 1u, 1u, 99u), 17);
    EXPECT_EQ(expr::evaluate(image.channel(0), 1L, 1L, 1L, 99L), 17);
    EXPECT_EQ(expr::evaluate(pixels, 1L, 1L, 1L)[0], 17);
    EXPECT_EQ(expr::evaluate(image * 2, 1L, 1L, 1L, 99L), 34);
    EXPECT_EQ(NearestInterpolator{}.interpolate(view, 1.0f, 1.0f, 1), 17);

    const Pixel<int32_t, 2> pixel{7, 11};
    EXPECT_EQ(expr::evaluate(pixel, 99L, 88L, 77L, 1L), 11);
    const Matrix<2, 2> matrix{{1, 2}, {3, 4}};
    EXPECT_EQ(&expr::evaluate(matrix, 99L, 88L), &matrix);
    EXPECT_EQ(&expr::evaluate(matrix, 99L, 88L, 77L, 66L), &matrix);
}

/// Checks invocation uses the same const qualification as the object passed to evaluate.
TEST(EvaluateTest, ConstInvocation) {
    const auto mutableCallable = [value = 7](int, int) mutable { return value; };
    EXPECT_EQ(&expr::evaluate(mutableCallable, 0, 0), &mutableCallable);
    EXPECT_EQ(&expr::evaluate(mutableCallable, 0, 0, 0, 0), &mutableCallable);

    const auto constCallable = [](int x, int y) { return x + y; };
    EXPECT_EQ(expr::evaluate(constCallable, 1, 2), 3);
    EXPECT_EQ(expr::evaluate(constCallable, 1, 2, 99, 88), 3);
}

/// Checks XY pixel callables broadcast over depth and XYZ pixel callables retain complete pixels.
TEST(EvaluateTest, PixelValuedCallables) {
    const auto spatial = [](int x, int y) { return Pixel<int32_t, 2>{x, y}; };
    const auto volume = [](int x, int y, int z) { return Pixel<int32_t, 2>{x + 100 * z, y + 100 * z}; };
    EXPECT_EQ(expr::evaluate(spatial, 1, 2, 3)[1], 2);
    EXPECT_EQ(expr::evaluate(volume, 1, 2, 3)[1], 302);
    EXPECT_EQ(expr::evaluate(expr::shift(volume, 1, 0), 0, 2, 3)[0], 301);

    Image32i image(LayoutDescriptor::Builder(2, 2, 2).numChannels(2).build(), 0);
    PixelView<int32_t, 2> pixels(image.descriptor());
    pixels = volume;
    EXPECT_EQ(image(1, 1, 1, 0), 101);
    EXPECT_EQ(image(1, 1, 1, 1), 101);

    pixels = spatial;
    EXPECT_EQ(image(1, 0, 1, 0), 1);
    EXPECT_EQ(image(1, 0, 1, 1), 0);
}

/// Checks whole-pixel matrix transforms combine all input channels at the requested slice.
TEST(EvaluateTest, MatrixPixelVolumes) {
    std::array<int32_t, 16> data;
    std::iota(data.begin(), data.end(), 10);
    const Image32i image(LayoutDescriptor::Builder(2, 2, 2).numChannels(2).build(), data.data());
    const PixelView<int32_t, 2> pixels(image.descriptor());
    const Matrix<3, 2> matrix{{1, 2}, {3, 4}, {5, 6}};
    const auto transformed = matrix * pixels;
    const auto pixel = expr::evaluate(transformed, 1, 1, 1);
    EXPECT_EQ(pixel[0], 67);
    EXPECT_EQ(pixel[1], 151);
    EXPECT_EQ(pixel[2], 235);
    EXPECT_EQ(expr::evaluate(transformed, 1L, 1L, 1L)[1], 151);

    Image32i output(LayoutDescriptor::Builder(2, 2, 2).numChannels(3).build(), 0);
    PixelView<int32_t, 3> outputPixels(output.descriptor());
    outputPixels = expr::cast<int32_t>(transformed + Pixel<int32_t, 3>{1, 2, 3});
    EXPECT_EQ(output(1, 1, 1, 0), 68);
    EXPECT_EQ(output(1, 1, 1, 1), 153);
    EXPECT_EQ(output(1, 1, 1, 2), 238);

    const Image32i scale(LayoutDescriptor::Builder(2, 2, 2).numChannels(1).build(), 2);
    EXPECT_EQ(expr::evaluate((scale * matrix) * pixels, 1, 1, 1)[2], 470);
}
