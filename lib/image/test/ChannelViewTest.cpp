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

#include <algorithm>
#include <numeric>

#include <gtest/gtest.h>

using namespace cxximg;

constexpr int W = 2;
constexpr int H = 2;
constexpr int N = 2;

template <typename T>
struct ChannelViewTest : public ::testing::Test {
    void SetUp() override {
        // Given I have a WxHxN image filled with zero
        memset(image.data(), 0, image.size() * sizeof(T));
    }

    Image<T> image = Image<T>(LayoutDescriptor::Builder(W, H).numChannels(N).build());
    ChannelView<T> channel = image.channels()[1]; // take second image channel
};

using ImageTypes = ::testing::Types<int8_t, int16_t, uint8_t, uint16_t, float, double>;
TYPED_TEST_SUITE(ChannelViewTest, ImageTypes);

TYPED_TEST(ChannelViewTest, TestSetValue) {
    // When I set (1, 0) to 1
    this->channel(1, 0) = TypeParam(1);

    // Then (1, 0, 1) value is 1 and other values are 0
    this->image.forEach([&](int x, int y, int z, int c) {
        if (x == 1 && y == 0 && c == 1) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(0));
        }
    });
}

TYPED_TEST(ChannelViewTest, TestAssignScalar) {
    // When I assign 1
    this->channel = 1;

    // Then channel values are 1 and other values are 0
    this->image.forEach([&](int x, int y, int z, int c) {
        if (c == 1) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(1));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(0));
        }
    });
}

TYPED_TEST(ChannelViewTest, TestAssignExpression) {
    // When I assign an expression (0 + 1) * (0 + 2) * 3 = 6
    this->channel = (this->channel + 1) * (this->channel + 2) * 3;

    // Then channel values are 6 and other values are 0
    this->image.forEach([&](int x, int y, int z, int c) {
        if (c == 1) {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(6));
        } else {
            ASSERT_EQ(this->image(x, y, z, c), TypeParam(0));
        }
    });
}

/// Checks channel assignment and statistics across logical slices while excluding padded storage.
TEST(ChannelVolumeTest, AssignAndMeasure) {
    for (const auto layoutKind : {ImageLayout::PLANAR, ImageLayout::INTERLEAVED}) {
        SCOPED_TRACE(toString(layoutKind));
        const auto layout = LayoutDescriptor::Builder(2, 2, 3)
                                    .numChannels(2)
                                    .imageLayout(layoutKind)
                                    .border(1)
                                    .widthAlignment(8)
                                    .heightAlignment(4)
                                    .depthAlignment(4)
                                    .build();
        Image32i image(layout);
        std::fill_n(image.data(), image.size(), -50);
        auto channel = image.channel(1);
        channel = [](int x, int y, int z) { return 10 * z + 2 * y + x; };

        const Image32i flat(LayoutDescriptor::Builder(2, 2).numChannels(1).build(), 2);
        channel += flat;
        channel -= 1;
        channel *= 2;
        channel /= 2;
        auto other = image.channel(0);
        other = channel + flat.channel(0);

        EXPECT_EQ(channel.size(), 12);
        EXPECT_EQ(channel(1, 1, 2), 24);
        EXPECT_EQ(other(1, 1, 2), 26);
        EXPECT_EQ(channel.minimum(), 1);
        EXPECT_EQ(channel.maximum(), 24);
        EXPECT_FLOAT_EQ(channel.mean(), 12.5f);
        EXPECT_EQ(channel.buffer(1, 2), &channel(0, 1, 2));
        EXPECT_EQ(std::count_if(image.data(), image.data() + image.size(), [](int32_t value) { return value >= 0; }),
                  24);

        const auto histogram = channel.histogram(26, 0, 25);
        EXPECT_EQ(histogram.count(1), 1u);
        EXPECT_EQ(histogram.count(24), 1u);
        EXPECT_EQ(histogram.count(0), 0u);
        EXPECT_EQ(std::accumulate(histogram.begin(), histogram.end(), 0u), 12u);
    }
}

/// Checks odd extents are rounded up when subsampling all three spatial dimensions.
TEST(ChannelVolumeTest, SubsampledIteration) {
    const auto layout = LayoutDescriptor::Builder(5, 5, 5)
                                .numChannels(2)
                                .imageLayout(ImageLayout::CUSTOM)
                                .channelSubsample(1, 2)
                                .build();
    Image32i image(layout, 0);
    auto channel = image.channel(1);
    int visits = 0;
    channel.forEach([&](int x, int y, int z) {
        channel(x, y, z) = 10 * z + 2 * y + x;
        ++visits;
    });

    EXPECT_EQ(channel.width(), 2);
    EXPECT_EQ(channel.height(), 2);
    EXPECT_EQ(channel.depth(), 2);
    EXPECT_EQ(channel.size(), 8);
    EXPECT_EQ(visits, 8);
    EXPECT_EQ(channel(1, 1, 1), 13);
}
