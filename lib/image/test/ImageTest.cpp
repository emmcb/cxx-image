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
#include "cxximg/image/detail/memory/StandardAllocator.h"

#include <algorithm>
#include <array>
#include <vector>

#include <gtest/gtest.h>

using namespace cxximg;

constexpr int W = 2;
constexpr int H = 2;
constexpr int N = 2;

/// Fills allocations so an unwritten volume slice has a deterministic sentinel value.
class FilledAllocator : public memory::detail::StandardAllocator {
public:
    /// Allocates aligned storage initialized to a value outside the test's pixel range.
    void* allocate(int64_t size) override {
        void* buffer = StandardAllocator::allocate(size);
        std::memset(buffer, 0xcc, size);
        return buffer;
    }
};

/// Keeps alignment-copy regression tests independent of allocator reuse and uninitialized storage.
class AlignmentVolumeTest : public ::testing::Test {
protected:
    /// Installs the allocator that makes missing writes observable.
    void SetUp() override { memory::detail::AllocatorManager::setCurrent(mAllocator); }

    /// Restores the allocator after all test-local images have been destroyed.
    void TearDown() override { memory::detail::AllocatorManager::setCurrent(mPreviousAllocator); }

private:
    memory::detail::Allocator& mPreviousAllocator = memory::detail::AllocatorManager::current();
    FilledAllocator mAllocator;
};

template <typename T>
struct ImageTest : public ::testing::Test {
    void SetUp() override {
        // Given I have a WxHxN image filled with zero
        memset(image.data(), 0, image.size() * sizeof(T));
    }

    Image<T> image = Image<T>(LayoutDescriptor::Builder(W, H).numChannels(N).build());
};

using ImageTypes = ::testing::Types<int8_t, int16_t, uint8_t, uint16_t, float, double>;
TYPED_TEST_SUITE(ImageTest, ImageTypes);

TYPED_TEST(ImageTest, TestSetValue) {
    // When I set (1, 0, 0) to 1
    this->image(1, 0, 0) = TypeParam(1);

    // Then (1, 0, 0) value is 1 and other values are 0
    this->image.forEach([&](int x, int y, int z, int c) {
        if (x == 1 && y == 0 && c == 0) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(0));
        }
    });
}

TYPED_TEST(ImageTest, TestAssignScalar) {
    // When I assign 1
    this->image = 1;

    // Then all the values are 1
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(1)); });
}

TYPED_TEST(ImageTest, TestAssignPixel) {
    // When I assign {1, 2}
    this->image = Pixel<int, N>{1, 2};

    // Then first channel is 1 and second channel is 2
    this->image.forEach([&](int x, int y, int z, int c) {
        if (c == 0) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(2));
        }
    });
}

TYPED_TEST(ImageTest, TestAssignExpression) {
    // When I assign an expression 3 * (0 + 1) * (0 + 2) = 6
    this->image = 3 * (this->image + 1) * (this->image + 2);

    // Then all the values are 6
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignExpressionTemporaryImage) {
    // When I construct an expression 3 * (0 + 1) * (0 + 2) = 6 using temporary images
    const auto expr = 3 * (this->image + Image<int>(LayoutDescriptor::Builder(W, H).numChannels(N).build(), 1)) *
                      (this->image + Image<int>(LayoutDescriptor::Builder(W, H).numChannels(N).build(), 2));

    // And latter assign this expression
    this->image = expr;

    // Then the temporary images lifetime has been extended and all the values are 6
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignExpressionTemporaryPixel) {
    // When I construct an expression 3 * (0 + 1) * (0 + 2) = 6 using temporary pixels
    const auto expr = 3 * (this->image + Pixel<int, N>(1)) * (this->image + Pixel<int, N>(2));

    // And latter assign this expression
    this->image = expr;

    // Then the temporary pixels lifetime has been extended and all the values are 6
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(6)); });
}

TYPED_TEST(ImageTest, TestAssignRoi) {
    // When I assign 1 to upper line
    this->image[{0, 0, W, 1}] = 1;

    // Then upper line values are 1 and bottom line values are 0
    this->image.forEach([&](int x, int y, int z, int c) {
        if (y == 0) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(0));
        }
    });

    // When I assign upper line to bottom line
    this->image[{0, 1, W, 1}] = this->image[{0, 0, W, 1}];

    // Then all the values are 1
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(1)); });
}

TYPED_TEST(ImageTest, TestClone) {
    // When I clone the image
    Image<TypeParam> clone = this->image.clone();

    // When I set (0, 0, 0) to 1
    clone(0, 0, 0) = TypeParam(1);

    // Then the dimensions of the clone are (W, H, N)
    ASSERT_EQ(clone.width(), W);
    ASSERT_EQ(clone.height(), H);
    ASSERT_EQ(clone.numChannels(), N);

    // Then (0, 0, 0) value of the clone is 1 and other values are 0
    clone.forEach([&](int x, int y, int z, int c) {
        if (x == 0 && y == 0 && c == 0) {
            ASSERT_EQ(clone(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(clone(x, y, z, c), TypeParam(0));
        }
    });

    // Then all original image values are 0
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(0)); });
}

TYPED_TEST(ImageTest, TestCloneRoi) {
    // When I clone an image ROI
    Image<TypeParam> clone = image::clone(this->image[{0, 0, 1, 1}]);

    // When I set (0, 0, 0) to 1
    clone(0, 0, 0) = TypeParam(1);

    // Then the dimensions of the clone are (W, 1, N)
    ASSERT_EQ(clone.width(), 1);
    ASSERT_EQ(clone.height(), 1);
    ASSERT_EQ(clone.numChannels(), N);

    // Then all original image values are 0
    clone.forEach([&](int x, int y, int z, int c) {
        if (c == 0) {
            ASSERT_EQ(clone(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(clone(x, y, z, c), TypeParam(0));
        }
    });

    // Then all original image values are 0
    this->image.forEach([&](int x, int y, int z, int c) { ASSERT_EQ(this->image(x, y, z, c), TypeParam(0)); });
}

/// Checks that volume iteration visits each logical pixel once and leaves borders and padding untouched.
TEST(ImageVolumeTest, ForEach) {
    for (const auto layoutKind : {ImageLayout::PLANAR, ImageLayout::INTERLEAVED}) {
        SCOPED_TRACE(toString(layoutKind));
        const auto layout = LayoutDescriptor::Builder(3, 2, 3)
                                    .numChannels(2)
                                    .imageLayout(layoutKind)
                                    .border(1)
                                    .widthAlignment(8)
                                    .heightAlignment(4)
                                    .depthAlignment(4)
                                    .build();
        Image32i image(layout);
        std::fill_n(image.data(), image.size(), -1);
        std::vector<int> visits(image.size(), 0);
        int numVisits = 0;

        image.forEach([&](int x, int y, int z, int c) {
            ASSERT_GE(x, 0);
            ASSERT_LT(x, 3);
            ASSERT_GE(y, 0);
            ASSERT_LT(y, 2);
            ASSERT_GE(z, 0);
            ASSERT_LT(z, 3);
            ASSERT_GE(c, 0);
            ASSERT_LT(c, 2);

            ++visits[&image(x, y, z, c) - image.data()];
            ++numVisits;
            image(x, y, z, c) = 1000 * c + 100 * z + 10 * y + x;
        });

        EXPECT_EQ(numVisits, 36);
        EXPECT_EQ(image(2, 1, 2, 1), 1212);
        for (int64_t i = 0; i < image.size(); ++i) {
            if (visits[i] == 0) {
                EXPECT_EQ(image[i], -1);
            } else {
                EXPECT_EQ(visits[i], 1);
                EXPECT_GE(image[i], 0);
            }
        }
    }
}

/// Checks odd-sized channels are traversed at their own subsampled XYZ resolution.
TEST(ImageVolumeTest, ForEachSubsampled) {
    const auto layout = LayoutDescriptor::Builder(5, 3, 3)
                                .imageLayout(ImageLayout::NV12)
                                .widthAlignment(4)
                                .heightAlignment(4)
                                .depthAlignment(4)
                                .build();
    Image32i image(layout);
    std::fill_n(image.data(), image.size(), -1);
    std::array<int, 3> visits = {0, 0, 0};

    image.forEach([&](int x, int y, int z, int c) {
        EXPECT_LT(x, c == 0 ? 5 : 3);
        EXPECT_LT(y, c == 0 ? 3 : 2);
        EXPECT_LT(z, c == 0 ? 3 : 2);
        ++visits[c];
        image(x, y, z, c) = 100 * c + 10 * z + 3 * y + x;
    });

    EXPECT_EQ(visits[0], 45);
    EXPECT_EQ(visits[1], 12);
    EXPECT_EQ(visits[2], 12);
    EXPECT_EQ(image(2, 1, 1, 2), 215);
    EXPECT_EQ(std::count_if(image.data(), image.data() + image.size(), [](int32_t value) { return value >= 0; }), 69);

    const auto customLayout = LayoutDescriptor::Builder(5, 5, 3)
                                      .imageLayout(ImageLayout::CUSTOM)
                                      .numChannels(3)
                                      .channelSubsample(1, 1)
                                      .channelSubsample(2, 2)
                                      .build();
    Image32i customImage(customLayout);
    visits.fill(0);
    customImage.forEach([&](int, int, int, int c) { ++visits[c]; });
    EXPECT_EQ(visits[0], 75);
    EXPECT_EQ(visits[1], 18);
    EXPECT_EQ(visits[2], 4);
}

/// Checks expression assignments, singleton broadcasting, and ROI cloning across volume slices.
TEST(ImageVolumeTest, AssignExpressions) {
    const auto layout = LayoutDescriptor::Builder(2, 2, 3)
                                .numChannels(2)
                                .widthAlignment(4)
                                .heightAlignment(4)
                                .depthAlignment(4)
                                .build();
    Image32i image(layout);
    std::fill_n(image.data(), image.size(), -7);
    image = [](int x, int y, int z, int c) { return 1000 * c + 100 * z + 10 * y + x; };

    const Image32i flat(LayoutDescriptor::Builder(2, 2).numChannels(1).build(), 2);
    image += flat;
    image -= Pixel<int32_t, 2>{1, 2};
    image *= 2;
    image /= 2;
    image = expr::abs(image * 2 - flat);

    image.forEach([&](int x, int y, int z, int c) {
        const int expected = 2 * (1000 * c + 100 * z + 10 * y + x + (c == 0 ? 1 : 0)) - 2;
        EXPECT_EQ(image(x, y, z, c), expected);
    });
    EXPECT_EQ(image[3], -7);  // Width padding.
    EXPECT_EQ(image[48], -7); // Depth padding.

    const auto clone = image::clone(image[Rect{1, 1, 1, 1}]);
    EXPECT_EQ(clone.depth(), 3);
    EXPECT_EQ(clone(0, 0, 2, 0), 422);
    EXPECT_EQ(clone(0, 0, 2, 1), 2420);

    image = [](int x, int y, int z) { return 100 * z + 10 * y + x; };
    EXPECT_EQ(image(1, 1, 2, 1), 211);
}

/// Checks alignment conversion preserves every logical slice, including odd subsampled dimensions.
TEST_F(AlignmentVolumeTest, CopiesEverySlice) {
    for (const auto layoutKind : {ImageLayout::PLANAR,
                                  ImageLayout::INTERLEAVED,
                                  ImageLayout::YUV_420,
                                  ImageLayout::NV12,
                                  ImageLayout::CUSTOM}) {
        SCOPED_TRACE(toString(layoutKind));
        auto builder = LayoutDescriptor::Builder(5, 3, 5)
                               .numChannels(3)
                               .imageLayout(layoutKind)
                               .depthAlignment(2)
                               .sizeAlignment(64);
        if (layoutKind == ImageLayout::CUSTOM) {
            builder.channelStrides(0, 8, 1, 32).channelStrides(1, 12, 2, 48).channelStrides(2, 16, 3, 64);
        }

        Image8u source(builder.build());
        source = [](int x, int y, int z, int c) { return uint8_t(40 * c + 20 * z + 5 * y + x); };
        const auto aligned = image::convertAlignment(source, 4, 4, 256);

        ASSERT_EQ(aligned.depth(), source.depth());
        source.forEach([&](int x, int y, int z, int c) {
            EXPECT_EQ(aligned(x, y, z, c), source(x, y, z, c))
                    << "Coordinates: " << x << ',' << y << ',' << z << ',' << c;
        });
    }
}

/// Checks channel flattening retains internal slice padding while excluding trailing height and depth padding.
TEST(ImageVolumeTest, FlattenChannelsWithSlicePadding) {
    const auto layout = LayoutDescriptor::Builder(3, 2, 3)
                                .numChannels(2)
                                .widthAlignment(4)
                                .heightAlignment(4)
                                .depthAlignment(4)
                                .sizeAlignment(256)
                                .build();
    Image32i image(layout);
    std::fill_n(image.data(), image.size(), -99);
    image = [](int x, int y, int z, int c) { return 1000 * c + 100 * z + 10 * y + x; };

    // Mark trailing padding above the logical maximum so including it would change the result.
    for (int c = 0; c < image.numChannels(); ++c) {
        std::fill_n(image.buffer(c) + 40, 24, 9999);
    }

    const auto flattened = image.flattenChannels();
    EXPECT_EQ(flattened.width(), 40);
    EXPECT_EQ(flattened.height(), 2);
    EXPECT_EQ(flattened.depth(), 1);
    EXPECT_EQ(flattened.layoutDescriptor().channels[0].rowStride, 64);
    EXPECT_EQ(flattened.layoutDescriptor().requiredBufferSize(), image.size());
    EXPECT_EQ(flattened(8, 0, 0), -99);
    EXPECT_EQ(flattened.maximum(), image.maximum());

    image.forEach([&](int x, int y, int z, int c) {
        const int offset = z * 16 + y * 4 + x;
        EXPECT_EQ(flattened(offset, c, 0), image(x, y, z, c));
    });
}

/// Checks channel flattening preserves 2D behavior and rejects spans that cannot fit an image width.
TEST(ImageVolumeTest, FlattenChannelsWidthLimits) {
    const Image32i image(LayoutDescriptor::Builder(3, 2).numChannels(2).heightAlignment(4).build(), 7);
    const auto flattened = image.flattenChannels();
    EXPECT_EQ(flattened.width(), 6);
    EXPECT_EQ(flattened.height(), 2);
    EXPECT_EQ(flattened.maximum(), 7);

    const ImageView8u large(LayoutDescriptor::Builder(65536, 65536, 2).numChannels(2).build(), nullptr);
    EXPECT_THROW(large.flattenChannels(), std::overflow_error);
}
