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

#include "gtest/gtest.h"

using namespace cxximg;

constexpr int W = 2;
constexpr int H = 2;
constexpr int N = 2;

template <typename T>
struct ImageTest : public ::testing::Test {
    void SetUp() override {
        // Given I have a WxHxN image filled with zero
        memset(image.data(), 0, image.size() * sizeof(T));
    }

    Image<T> image = Image<T>(LayoutDescriptor::Builder(W, H).numPlanes(N).build());
};

using ImageTypes = ::testing::Types<int8_t, int16_t, uint8_t, uint16_t, float, double>;
TYPED_TEST_SUITE(ImageTest, ImageTypes);

TYPED_TEST(ImageTest, TestSetValue) {
    // When I set (1, 0, 0) to 1
    this->image(1, 0, 0) = TypeParam(1);

    // Then (1, 0, 0) value is 1 and other values are 0
    this->image.forEach([&](int x, int y, int n) {
        if (x == 1 && y == 0 && n == 0) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(0));
        }
    });
}

TYPED_TEST(ImageTest, TestAssignScalar) {
    // When I assign 1
    this->image = 1;

    // Then all the values are 1
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(1)); });
}

TYPED_TEST(ImageTest, TestAssignPixel) {
    // When I assign {1, 2}
    this->image = Pixel<int, N>{1, 2};

    // Then first plane is 1 and second plane is 2
    this->image.forEach([&](int x, int y, int n) {
        if (n == 0) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(2));
        }
    });
}

TYPED_TEST(ImageTest, TestAssignExpression) {
    // When I assign an expression 3 * (0 + 1) * (0 + 2) = 6
    this->image = 3 * (this->image + 1) * (this->image + 2);

    // Then all the values are 6
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignExpressionTemporaryImage) {
    // When I construct an expression 3 * (0 + 1) * (0 + 2) = 6 using temporary images
    const auto expr = 3 * (this->image + Image<int>(LayoutDescriptor::Builder(W, H).numPlanes(N).build(), 1)) *
                      (this->image + Image<int>(LayoutDescriptor::Builder(W, H).numPlanes(N).build(), 2));

    // And latter assign this expression
    this->image = expr;

    // Then the temporary images lifetime has been extended and all the values are 6
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignExpressionTemporaryPixel) {
    // When I construct an expression 3 * (0 + 1) * (0 + 2) = 6 using temporary pixels
    const auto expr = 3 * (this->image + Pixel<int, N>(1)) * (this->image + Pixel<int, N>(2));

    // And latter assign this expression
    this->image = expr;

    // Then the temporary pixels lifetime has been extended and all the values are 6
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignRoi) {
    // When I assign 1 to upper line
    this->image[{0, 0, W, 1}] = 1;

    // Then upper line values are 1 and bottom line values are 0
    this->image.forEach([&](int x, int y, int n) {
        if (y == 0) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(0));
        }
    });

    // When I assign upper line to bottom line
    this->image[{0, 1, W, 1}] = this->image[{0, 0, W, 1}];

    // Then all the values are 1
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(1)); });
}

TYPED_TEST(ImageTest, TestClone) {
    // When I clone the image
    Image<TypeParam> clone = this->image.clone();

    // When I set (0, 0, 0) to 1
    clone(0, 0, 0) = TypeParam(1);

    // Then the dimensions of the clone are (W, H, N)
    ASSERT_EQ(clone.width(), W);
    ASSERT_EQ(clone.height(), H);
    ASSERT_EQ(clone.numPlanes(), N);

    // Then (0, 0, 0) value of the clone is 1 and other values are 0
    clone.forEach([&](int x, int y, int n) {
        if (x == 0 && y == 0 && n == 0) {
            ASSERT_EQ(clone(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(clone(x, y, n), TypeParam(0));
        }
    });

    // Then all original image values are 0
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(0)); });
}

TYPED_TEST(ImageTest, TestCloneRoi) {
    // When I clone an image ROI
    Image<TypeParam> clone = image::clone(this->image[{0, 0, 1, 1}]);

    // When I set (0, 0, 0) to 1
    clone(0, 0, 0) = TypeParam(1);

    // Then the dimensions of the clone are (W, 1, N)
    ASSERT_EQ(clone.width(), 1);
    ASSERT_EQ(clone.height(), 1);
    ASSERT_EQ(clone.numPlanes(), N);

    // Then all original image values are 0
    clone.forEach([&](int x, int y, int n) {
        if (n == 0) {
            ASSERT_EQ(clone(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(clone(x, y, n), TypeParam(0));
        }
    });

    // Then all original image values are 0
    this->image.forEach([&](int x, int y, int n) { ASSERT_EQ(this->image(x, y, n), TypeParam(0)); });
}
