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

#include "image/LayoutDescriptor.h"

#include "gtest/gtest.h"

using namespace cxximg;

TEST(LayoutTest, TestLayoutAlignment) {
    // PLANAR layout, forcing alignment to 8
    LayoutDescriptor layout = LayoutDescriptor::Builder(20, 20).numPlanes(1).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 480); // 24 x 20

    // PLANAR layout, forcing alignment to 1
    layout = LayoutDescriptor::Builder(20, 20).numPlanes(1).widthAlignment(1).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 20);
    ASSERT_EQ(layout.requiredBufferSize(), 400); // 20 x 20

    // PLANAR layout, forcing alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).numPlanes(1).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 512); // 16 x 32

    // NV12 layout with even dimensions and forcing alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).imageLayout(ImageLayout::NV12).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 768); // (16 + 8) * 32

    // NV12 layout with odd dimensions and forcing alignment to 8
    layout = LayoutDescriptor::Builder(17, 13).imageLayout(ImageLayout::NV12).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 504); // (24 + 12) * 14

    // YUV_420 layout with even dimensions and forcing alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).imageLayout(ImageLayout::YUV_420).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 768); // (16 + 8) * 32

    // YUV_420 layout with odd dimensions and forcing alignment to 8
    layout = LayoutDescriptor::Builder(17, 13).imageLayout(ImageLayout::YUV_420).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 560); // (24 + 16) * 14

    ASSERT_THROW(LayoutDescriptor::Builder(2, 2).numPlanes(1).widthAlignment(0).build(), std::invalid_argument);
}
