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
#include "cxximg/image/function/Border.h"

#include <gtest/gtest.h>

using namespace cxximg;

constexpr int W = 3;
constexpr int H = 3;
constexpr int N = 1;

struct BorderTest : public ::testing::Test {
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

TEST_F(BorderTest, TestMakeBorderConstant) {
    // For constant mode, 3x3 image with 2px border around should give:
    // 0 0  0 0 0  0 0
    // 0 0  0 0 0  0 0
    //
    // 0 0  a b c  0 0
    // 0 0  d e f  0 0
    // 0 0  g h i  0 0
    //
    // 0 0  0 0 0  0 0
    // 0 0  0 0 0  0 0

    // When I create constant borders filled with zero
    Image8u borders = image::makeBorders<BorderMode::CONSTANT>(image, 2);

    // Then dimensions are correct
    ASSERT_EQ(borders.width(), W);
    ASSERT_EQ(borders.height(), H);
    ASSERT_EQ(borders.numPlanes(), N);

    // Then values are correct

    // Line 1
    int y = -2;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 0);
    ASSERT_EQ(borders(1, y, 0), 0);
    ASSERT_EQ(borders(2, y, 0), 0);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 2
    y = -1;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 0);
    ASSERT_EQ(borders(1, y, 0), 0);
    ASSERT_EQ(borders(2, y, 0), 0);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 3
    y = 0;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 4
    y = 1;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 5
    y = 2;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 6
    y = 3;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 0);
    ASSERT_EQ(borders(1, y, 0), 0);
    ASSERT_EQ(borders(2, y, 0), 0);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);

    // Line 7
    y = 4;
    ASSERT_EQ(borders(-2, y, 0), 0);
    ASSERT_EQ(borders(-1, y, 0), 0);
    ASSERT_EQ(borders(0, y, 0), 0);
    ASSERT_EQ(borders(1, y, 0), 0);
    ASSERT_EQ(borders(2, y, 0), 0);
    ASSERT_EQ(borders(3, y, 0), 0);
    ASSERT_EQ(borders(4, y, 0), 0);
}

TEST_F(BorderTest, TestMakeBorderMirror) {
    // For mirror mode, 3x3 image with 2px border around should give:
    // i h  g h i  h g
    // f e  d e f  e d
    //
    // c b  a b c  b a
    // f e  d e f  e d
    // i h  g h i  h g
    //
    // f e  d e f  e d
    // c b  a b c  b a

    // When I create mirror borders
    Image8u borders = image::makeBorders<BorderMode::MIRROR>(image, 2);

    // Then dimensions are correct
    ASSERT_EQ(borders.width(), W);
    ASSERT_EQ(borders.height(), H);
    ASSERT_EQ(borders.numPlanes(), N);

    // Then values are correct

    // Line 1
    int y = -2;
    ASSERT_EQ(borders(-2, y, 0), 9);
    ASSERT_EQ(borders(-1, y, 0), 8);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 8);
    ASSERT_EQ(borders(4, y, 0), 7);

    // Line 2
    y = -1;
    ASSERT_EQ(borders(-2, y, 0), 6);
    ASSERT_EQ(borders(-1, y, 0), 5);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 5);
    ASSERT_EQ(borders(4, y, 0), 4);

    // Line 3
    y = 0;
    ASSERT_EQ(borders(-2, y, 0), 3);
    ASSERT_EQ(borders(-1, y, 0), 2);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 2);
    ASSERT_EQ(borders(4, y, 0), 1);

    // Line 4
    y = 1;
    ASSERT_EQ(borders(-2, y, 0), 6);
    ASSERT_EQ(borders(-1, y, 0), 5);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 5);
    ASSERT_EQ(borders(4, y, 0), 4);

    // Line 5
    y = 2;
    ASSERT_EQ(borders(-2, y, 0), 9);
    ASSERT_EQ(borders(-1, y, 0), 8);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 8);
    ASSERT_EQ(borders(4, y, 0), 7);

    // Line 6
    y = 3;
    ASSERT_EQ(borders(-2, y, 0), 6);
    ASSERT_EQ(borders(-1, y, 0), 5);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 5);
    ASSERT_EQ(borders(4, y, 0), 4);

    // Line 7
    y = 4;
    ASSERT_EQ(borders(-2, y, 0), 3);
    ASSERT_EQ(borders(-1, y, 0), 2);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 2);
    ASSERT_EQ(borders(4, y, 0), 1);
}

TEST_F(BorderTest, TestMakeBorderNearest) {
    // For nearest mode, 3x3 image with 2px border around should give:
    // a a  a b c  c c
    // a a  a b c  c c
    //
    // a a  a b c  c c
    // d d  d e f  f f
    // g g  g h i  i i
    //
    // g g  g h i  i i
    // g g  g h i  i i

    // When I create nearest borders
    Image8u borders = image::makeBorders<BorderMode::NEAREST>(image, 2);

    // Then dimensions are correct
    ASSERT_EQ(borders.width(), W);
    ASSERT_EQ(borders.height(), H);
    ASSERT_EQ(borders.numPlanes(), N);

    // Then values are correct

    // Line 1
    int y = -2;
    ASSERT_EQ(borders(-2, y, 0), 1);
    ASSERT_EQ(borders(-1, y, 0), 1);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 3);
    ASSERT_EQ(borders(4, y, 0), 3);

    // Line 2
    y = -1;
    ASSERT_EQ(borders(-2, y, 0), 1);
    ASSERT_EQ(borders(-1, y, 0), 1);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 3);
    ASSERT_EQ(borders(4, y, 0), 3);

    // Line 3
    y = 0;
    ASSERT_EQ(borders(-2, y, 0), 1);
    ASSERT_EQ(borders(-1, y, 0), 1);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 3);
    ASSERT_EQ(borders(4, y, 0), 3);

    // Line 4
    y = 1;
    ASSERT_EQ(borders(-2, y, 0), 4);
    ASSERT_EQ(borders(-1, y, 0), 4);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 6);
    ASSERT_EQ(borders(4, y, 0), 6);

    // Line 5
    y = 2;
    ASSERT_EQ(borders(-2, y, 0), 7);
    ASSERT_EQ(borders(-1, y, 0), 7);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 9);
    ASSERT_EQ(borders(4, y, 0), 9);

    // Line 6
    y = 3;
    ASSERT_EQ(borders(-2, y, 0), 7);
    ASSERT_EQ(borders(-1, y, 0), 7);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 9);
    ASSERT_EQ(borders(4, y, 0), 9);

    // Line 7
    y = 4;
    ASSERT_EQ(borders(-2, y, 0), 7);
    ASSERT_EQ(borders(-1, y, 0), 7);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 9);
    ASSERT_EQ(borders(4, y, 0), 9);
}

TEST_F(BorderTest, TestMakeBorderReflect) {
    // For reflect mode, 3x3 image with 2px border around should give:
    // e d  d e f  f e
    // b a  a b c  c b
    //
    // b a  a b c  c b
    // e d  d e f  f e
    // h g  g h i  i h
    //
    // h g  g h i  i h
    // e d  d e f  f e

    // When I create reflect borders
    Image8u borders = image::makeBorders<BorderMode::REFLECT>(image, 2);

    // Then dimensions are correct
    ASSERT_EQ(borders.width(), W);
    ASSERT_EQ(borders.height(), H);
    ASSERT_EQ(borders.numPlanes(), N);

    // Then values are correct

    // Line 1
    int y = -2;
    ASSERT_EQ(borders(-2, y, 0), 5);
    ASSERT_EQ(borders(-1, y, 0), 4);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 6);
    ASSERT_EQ(borders(4, y, 0), 5);

    // Line 2
    y = -1;
    ASSERT_EQ(borders(-2, y, 0), 2);
    ASSERT_EQ(borders(-1, y, 0), 1);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 3);
    ASSERT_EQ(borders(4, y, 0), 2);

    // Line 3
    y = 0;
    ASSERT_EQ(borders(-2, y, 0), 2);
    ASSERT_EQ(borders(-1, y, 0), 1);
    ASSERT_EQ(borders(0, y, 0), 1);
    ASSERT_EQ(borders(1, y, 0), 2);
    ASSERT_EQ(borders(2, y, 0), 3);
    ASSERT_EQ(borders(3, y, 0), 3);
    ASSERT_EQ(borders(4, y, 0), 2);

    // Line 4
    y = 1;
    ASSERT_EQ(borders(-2, y, 0), 5);
    ASSERT_EQ(borders(-1, y, 0), 4);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 6);
    ASSERT_EQ(borders(4, y, 0), 5);

    // Line 5
    y = 2;
    ASSERT_EQ(borders(-2, y, 0), 8);
    ASSERT_EQ(borders(-1, y, 0), 7);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 9);
    ASSERT_EQ(borders(4, y, 0), 8);

    // Line 6
    y = 3;
    ASSERT_EQ(borders(-2, y, 0), 8);
    ASSERT_EQ(borders(-1, y, 0), 7);
    ASSERT_EQ(borders(0, y, 0), 7);
    ASSERT_EQ(borders(1, y, 0), 8);
    ASSERT_EQ(borders(2, y, 0), 9);
    ASSERT_EQ(borders(3, y, 0), 9);
    ASSERT_EQ(borders(4, y, 0), 8);

    // Line 7
    y = 4;
    ASSERT_EQ(borders(-2, y, 0), 5);
    ASSERT_EQ(borders(-1, y, 0), 4);
    ASSERT_EQ(borders(0, y, 0), 4);
    ASSERT_EQ(borders(1, y, 0), 5);
    ASSERT_EQ(borders(2, y, 0), 6);
    ASSERT_EQ(borders(3, y, 0), 6);
    ASSERT_EQ(borders(4, y, 0), 5);
}
