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
struct PlaneViewTest : public ::testing::Test {
    void SetUp() override {
        // Given I have a WxHxN image filled with zero
        memset(image.data(), 0, image.size() * sizeof(T));
    }

    Image<T> image = Image<T>(LayoutDescriptor::Builder(W, H).numPlanes(N).build());
    PlaneView<T> plane = image.planes()[1]; // take second image plane
};

using ImageTypes = ::testing::Types<int8_t, int16_t, uint8_t, uint16_t, float, double>;
TYPED_TEST_SUITE(PlaneViewTest, ImageTypes);

TYPED_TEST(PlaneViewTest, TestSetValue) {
    // When I set (1, 0) to 1
    this->plane(1, 0) = TypeParam(1);

    // Then (1, 0, 1) value is 1 and other values are 0
    this->image.forEach([&](int x, int y, int n) {
        if (x == 1 && y == 0 && n == 1) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(0));
        }
    });
}

TYPED_TEST(PlaneViewTest, TestAssignScalar) {
    // When I assign 1
    this->plane = 1;

    // Then plane values are 1 and other values are 0
    this->image.forEach([&](int x, int y, int n) {
        if (n == 1) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(0));
        }
    });
}

TYPED_TEST(PlaneViewTest, TestAssignExpression) {
    // When I assign an expression (0 + 1) * (0 + 2) * 3 = 6
    this->plane = (this->plane + 1) * (this->plane + 2) * 3;

    // Then plane values are 6 and other values are 0
    this->image.forEach([&](int x, int y, int n) {
        if (n == 1) {
            ASSERT_EQ(this->image(x, y, n), TypeParam(6));
        } else {
            ASSERT_EQ(this->image(x, y, n), TypeParam(0));
        }
    });
}
