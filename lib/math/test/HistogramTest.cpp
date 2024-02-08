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

#include "math/Histogram.h"

#include "gtest/gtest.h"

using namespace cxximg;

TEST(HistogramTest, TestRegularAxisFloat) {
    hist::RegularAxis<float> a(4, -2.0f, 2.0f);

    // ASSERT_FLOAT_EQ(a.interval(-1).lower(),
    // std::numeric_limits<float>::lowest());
    ASSERT_FLOAT_EQ(a.interval(-1).upper(), -2.0f);
    ASSERT_FLOAT_EQ(a.interval(0).lower(), -2.0f);
    ASSERT_FLOAT_EQ(a.interval(0).upper(), -1.0f);
    ASSERT_FLOAT_EQ(a.interval(1).lower(), -1.0f);
    ASSERT_FLOAT_EQ(a.interval(1).upper(), -0.0f);
    ASSERT_FLOAT_EQ(a.interval(2).lower(), 0.0f);
    ASSERT_FLOAT_EQ(a.interval(2).upper(), 1.0f);
    ASSERT_FLOAT_EQ(a.interval(3).lower(), 1.0f);
    ASSERT_FLOAT_EQ(a.interval(3).upper(), 2.0f);
    ASSERT_FLOAT_EQ(a.interval(4).lower(), 2.0f);
    // ASSERT_FLOAT_EQ(a.interval(4).upper(), std::numeric_limits<float>::max());
    ASSERT_EQ(a.index(-10.0f), -1);
    ASSERT_EQ(a.index(-2.1f), -1);
    ASSERT_EQ(a.index(-2.0f), 0);
    ASSERT_EQ(a.index(-1.1f), 0);
    ASSERT_EQ(a.index(0.0f), 2);
    ASSERT_EQ(a.index(0.9f), 2);
    ASSERT_EQ(a.index(1.0f), 3);
    ASSERT_EQ(a.index(2.0f), 3);
    ASSERT_EQ(a.index(10.0f), 4);
}

TEST(HistogramTest, TestRegularAxisInt) {
    hist::RegularAxis<int> a(256, 0, 255);

    // ASSERT_EQ(a.interval(-1).lower(), std::numeric_limits<int>::lowest());
    ASSERT_EQ(a.interval(-1).upper(), 0);
    ASSERT_EQ(a.interval(0).lower(), 0);
    ASSERT_EQ(a.interval(0).upper(), 1);
    ASSERT_EQ(a.interval(1).lower(), 1);
    ASSERT_EQ(a.interval(1).upper(), 2);
    ASSERT_EQ(a.interval(254).lower(), 254);
    ASSERT_EQ(a.interval(254).upper(), 255);
    ASSERT_EQ(a.interval(255).lower(), 255);
    ASSERT_EQ(a.interval(255).upper(), 256);
    ASSERT_EQ(a.interval(256).lower(), 256);
    // ASSERT_EQ(a.interval(256).upper(), std::numeric_limits<int>::max());
    ASSERT_EQ(a.index(-10), -1);
    ASSERT_EQ(a.index(-1), -1);
    ASSERT_EQ(a.index(0), 0);
    ASSERT_EQ(a.index(1), 1);
    ASSERT_EQ(a.index(254), 254);
    ASSERT_EQ(a.index(255), 255);
    ASSERT_EQ(a.index(256), 256);
    ASSERT_EQ(a.index(300), 256);
}

TEST(HistogramTest, TestRegularAxisUInt8) {
    hist::RegularAxis<uint8_t> a(256, 0, 255);

    ASSERT_EQ(a.interval(-1).lower(), 0);
    ASSERT_EQ(a.interval(-1).upper(), 0);
    ASSERT_EQ(a.interval(0).lower(), 0);
    ASSERT_EQ(a.interval(0).upper(), 1);
    ASSERT_EQ(a.interval(1).lower(), 1);
    ASSERT_EQ(a.interval(1).upper(), 2);
    ASSERT_EQ(a.interval(254).lower(), 254);
    ASSERT_EQ(a.interval(254).upper(), 255);
    ASSERT_EQ(a.interval(255).lower(), 255);
    ASSERT_EQ(a.interval(255).upper(), 255);
    ASSERT_EQ(a.interval(256).lower(), 255);
    ASSERT_EQ(a.interval(256).upper(), 255);
    ASSERT_EQ(a.index(0), 0);
    ASSERT_EQ(a.index(1), 1);
    ASSERT_EQ(a.index(254), 254);
    ASSERT_EQ(a.index(255), 255);
}

TEST(HistogramTest, TestRegularAxisUInt16) {
    hist::RegularAxis<uint16_t> a(256, 0, 65535);

    ASSERT_EQ(a.interval(-1).lower(), 0);
    ASSERT_EQ(a.interval(-1).upper(), 0);
    ASSERT_EQ(a.interval(0).lower(), 0);
    ASSERT_EQ(a.interval(0).upper(), 256);
    ASSERT_EQ(a.interval(1).lower(), 256);
    ASSERT_EQ(a.interval(1).upper(), 512);
    ASSERT_EQ(a.interval(254).lower(), 65024);
    ASSERT_EQ(a.interval(254).upper(), 65280);
    ASSERT_EQ(a.interval(255).lower(), 65280);
    ASSERT_EQ(a.interval(255).upper(), 65535);
    ASSERT_EQ(a.interval(256).lower(), 65535);
    ASSERT_EQ(a.interval(256).upper(), 65535);
    ASSERT_EQ(a.index(0), 0);
    ASSERT_EQ(a.index(255), 0);
    ASSERT_EQ(a.index(256), 1);
    ASSERT_EQ(a.index(511), 1);
    ASSERT_EQ(a.index(512), 2);
    ASSERT_EQ(a.index(65279), 254);
    ASSERT_EQ(a.index(65280), 255);
    ASSERT_EQ(a.index(65535), 255);
}

TEST(HistogramTest, TestHistogram1D) {
    auto h = hist::makeHistogram(hist::RegularAxis<float>(6, -1.0f, 2.0f));

    // fill histogram with data
    const auto data = {-0.5f, 1.1f, 0.3f, 1.7f};
    std::for_each(data.begin(), data.end(), std::ref(h));

    // fill values manually
    h(-1.5f);
    h(-1.0f);
    h(2.0f);
    h(20.0f);
    h(0.1f);

    ASSERT_EQ(h.totalCount(), 9u);
    ASSERT_EQ(h[-1], 1u);
    ASSERT_EQ(h[0], 1u);
    ASSERT_EQ(h[1], 1u);
    ASSERT_EQ(h[2], 2u);
    ASSERT_EQ(h[3], 0u);
    ASSERT_EQ(h[4], 1u);
    ASSERT_EQ(h[5], 2u);
    ASSERT_EQ(h[6], 1u);

    // check indexed access
    auto iterator = h.indexed().begin();
    for (int i = 0; i < h.axis().size(); ++i) {
        const auto accessor = *iterator;
        ASSERT_EQ(accessor.index(), i);
        ASSERT_EQ(*accessor, h.at(i));
        ++iterator;
    }
    ASSERT_EQ(iterator, h.indexed().end());

    // compute accumulated histogram
    const auto acc = h.accumulated();

    ASSERT_EQ(acc.totalCount(), 9u);
    ASSERT_EQ(acc[-1], 1u);
    ASSERT_EQ(acc[0], 2u);
    ASSERT_EQ(acc[1], 3u);
    ASSERT_EQ(acc[2], 5u);
    ASSERT_EQ(acc[3], 5u);
    ASSERT_EQ(acc[4], 6u);
    ASSERT_EQ(acc[5], 8u);
    ASSERT_EQ(acc[6], 9u);

    ASSERT_EQ(acc.count(1.25f), 6u);
    ASSERT_EQ(acc.count(1.75f), 8u);
    ASSERT_EQ(acc.coord(6u), 1.5f);
    ASSERT_EQ(acc.coord(7u), 1.75f);
    ASSERT_EQ(acc.coord(8u), 2.0f);
}

TEST(HistogramTest, TestHistogram2D) {
    auto h = hist::makeHistogram(hist::RegularAxis<float>(2, 0.0f, 1.0f), hist::RegularAxis<float>(2, 0.0f, 1.0f));

    // fill values manually
    h(-1.0f, -1.0f);
    h(-1.0f, 0.1f);
    h(0.1f, -1.0f);
    h(2.0f, 0.9f);
    h(0.9f, 2.0f);
    h(2.0f, 2.0f);
    h(0.1f, 0.2f);
    h(0.7f, 0.3f);
    h(0.3f, 0.7f);
    h(0.7f, 0.7f);

    ASSERT_EQ(h.totalCount(), 10u);
    ASSERT_EQ(h.at(-1, -1), 1u);
    ASSERT_EQ(h.at(-1, 0), 1u);
    ASSERT_EQ(h.at(-1, 1), 0u);
    ASSERT_EQ(h.at(-1, 2), 0u);
    ASSERT_EQ(h.at(0, -1), 1u);
    ASSERT_EQ(h.at(0, 0), 1u);
    ASSERT_EQ(h.at(0, 1), 1u);
    ASSERT_EQ(h.at(0, 2), 0u);
    ASSERT_EQ(h.at(1, -1), 0u);
    ASSERT_EQ(h.at(1, 0), 1u);
    ASSERT_EQ(h.at(1, 1), 1u);
    ASSERT_EQ(h.at(1, 2), 1u);
    ASSERT_EQ(h.at(2, -1), 0u);
    ASSERT_EQ(h.at(2, 0), 0u);
    ASSERT_EQ(h.at(2, 1), 1u);
    ASSERT_EQ(h.at(2, 2), 1u);

    // check indexed access
    auto iterator = h.indexed().begin();
    for (int j = 0; j < h.axis<1>().size(); ++j) {
        for (int i = 0; i < h.axis<0>().size(); ++i) {
            const auto accessor = *iterator;
            ASSERT_EQ(accessor.index<0>(), i);
            ASSERT_EQ(accessor.index<1>(), j);
            ASSERT_EQ(*accessor, h.at(i, j));
            ++iterator;
        }
    }
    ASSERT_EQ(iterator, h.indexed().end());
}
