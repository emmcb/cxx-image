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

#include <gtest/gtest.h>

using namespace cxximg;

constexpr int W = 3;
constexpr int H = 3;
constexpr int N = 1;

struct ResizeExpressionTest : public ::testing::Test {
    void SetUp() override {
        // Given I have a WxHxN image filled with increasing numbers
        uint8_t i = 1;
        image.forEach([&](int x, int y, int n) {
            image(x, y, n) = i;
            ++i;
        });
    }

    Image8u image = Image8u(LayoutDescriptor::Builder(W, H).numPlanes(N).build());
};

TEST_F(ResizeExpressionTest, TestUpsampleCornerAligned) {
    // When I downsample the image
    Imagef out(LayoutDescriptor::Builder(4, 4).numPlanes(1).build(), expr::resize(image, 4, 4, true));

    // Then values are correct
    ASSERT_FLOAT_EQ(out(0, 0, 0), 1.0f);
    ASSERT_FLOAT_EQ(out(1, 0, 0), 1.0f + 2.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(2, 0, 0), 2.0f + 1.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(3, 0, 0), 3.0f);

    ASSERT_FLOAT_EQ(out(0, 1, 0), 3.0f);
    ASSERT_FLOAT_EQ(out(1, 1, 0), 3.0f + 2.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(2, 1, 0), 4.0f + 1.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(3, 1, 0), 5.0f);

    ASSERT_FLOAT_EQ(out(0, 2, 0), 5.0f);
    ASSERT_FLOAT_EQ(out(1, 2, 0), 5.0f + 2.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(2, 2, 0), 6.0f + 1.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(3, 2, 0), 7.0f);

    ASSERT_FLOAT_EQ(out(0, 3, 0), 7.0f);
    ASSERT_FLOAT_EQ(out(1, 3, 0), 7.0f + 2.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(2, 3, 0), 8.0f + 1.0f / 3.0f);
    ASSERT_FLOAT_EQ(out(3, 3, 0), 9.0f);
}

TEST_F(ResizeExpressionTest, TestUpsampleCenterAligned) {
    // When I upsample the image
    Imagef out(LayoutDescriptor::Builder(4, 4).numPlanes(1).build(), expr::resize(image, 4, 4, false));

    // Then values are correct
    ASSERT_FLOAT_EQ(out(0, 0, 0), 1.0f);
    ASSERT_FLOAT_EQ(out(1, 0, 0), 1.625f);
    ASSERT_FLOAT_EQ(out(2, 0, 0), 2.375f);
    ASSERT_FLOAT_EQ(out(3, 0, 0), 3.0f);

    ASSERT_FLOAT_EQ(out(0, 1, 0), 2.875f);
    ASSERT_FLOAT_EQ(out(1, 1, 0), 3.5f);
    ASSERT_FLOAT_EQ(out(2, 1, 0), 4.25f);
    ASSERT_FLOAT_EQ(out(3, 1, 0), 4.875f);

    ASSERT_FLOAT_EQ(out(0, 2, 0), 5.125f);
    ASSERT_FLOAT_EQ(out(1, 2, 0), 5.75f);
    ASSERT_FLOAT_EQ(out(2, 2, 0), 6.5f);
    ASSERT_FLOAT_EQ(out(3, 2, 0), 7.125f);

    ASSERT_FLOAT_EQ(out(0, 3, 0), 7.0f);
    ASSERT_FLOAT_EQ(out(1, 3, 0), 7.625f);
    ASSERT_FLOAT_EQ(out(2, 3, 0), 8.375f);
    ASSERT_FLOAT_EQ(out(3, 3, 0), 9.0f);
}

TEST_F(ResizeExpressionTest, TestDownsampleCornerAligned) {
    // When I downsample the image
    Imagef out(LayoutDescriptor::Builder(2, 2).numPlanes(1).build(), expr::resize(image, 2, 2, true));

    // Then values are correct
    ASSERT_FLOAT_EQ(out(0, 0, 0), 1.0f);
    ASSERT_FLOAT_EQ(out(1, 0, 0), 3.0f);
    ASSERT_FLOAT_EQ(out(0, 1, 0), 7.0f);
    ASSERT_FLOAT_EQ(out(1, 1, 0), 9.0f);
}

TEST_F(ResizeExpressionTest, TestDownsampleCenterAligned) {
    // When I downsample the image
    Imagef out(LayoutDescriptor::Builder(2, 2).numPlanes(1).build(), expr::resize(image, 2, 2, false));

    // Then values are correct
    ASSERT_FLOAT_EQ(out(0, 0, 0), 2.0f);
    ASSERT_FLOAT_EQ(out(1, 0, 0), 3.5f);
    ASSERT_FLOAT_EQ(out(0, 1, 0), 6.5f);
    ASSERT_FLOAT_EQ(out(1, 1, 0), 8.0f);
}
