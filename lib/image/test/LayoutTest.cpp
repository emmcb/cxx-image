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

#include "cxximg/image/ImageDescriptor.h"

#include <gtest/gtest.h>

using namespace cxximg;

/// Checks existing 2D layout alignment and buffer sizes.
TEST(LayoutTest, TestLayoutAlignment) {
    // PLANAR layout, forcing width alignment to 8
    LayoutDescriptor layout = LayoutDescriptor::Builder(20, 20).numChannels(1).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 480); // 24 x 20

    // PLANAR layout, forcing width alignment to 1
    layout = LayoutDescriptor::Builder(20, 20).numChannels(1).widthAlignment(1).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 20);
    ASSERT_EQ(layout.requiredBufferSize(), 400); // 20 x 20

    // PLANAR layout, forcing width alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).numChannels(1).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 512); // 16 x 32

    // NV12 layout with even dimensions and forcing width alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).imageLayout(ImageLayout::NV12).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 768); // (16 + 8) * 32

    // NV12 layout with odd dimensions and forcing width alignment to 8
    layout = LayoutDescriptor::Builder(17, 13).imageLayout(ImageLayout::NV12).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 504); // (24 + 12) * 14

    // YUV_420 layout with even dimensions and forcing width alignment to 8
    layout = LayoutDescriptor::Builder(16, 32).imageLayout(ImageLayout::YUV_420).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 16);
    ASSERT_EQ(layout.requiredBufferSize(), 768); // (16 + 8) * 32

    // YUV_420 layout with odd dimensions and forcing width alignment to 8
    layout = LayoutDescriptor::Builder(17, 13).imageLayout(ImageLayout::YUV_420).widthAlignment(8).build();
    ASSERT_EQ(detail::alignDimension(layout.width, layout.widthAlignment), 24);
    ASSERT_EQ(layout.requiredBufferSize(), 560); // (24 + 16) * 14

    ASSERT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).widthAlignment(0).build(), std::invalid_argument);
    ASSERT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).heightAlignment(0).build(), std::invalid_argument);
    ASSERT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).sizeAlignment(0).build(), std::invalid_argument);

    /// PLANAR layout, forcing buffer size alignment to 4096
    layout = LayoutDescriptor::Builder(4000, 3000).pixelType(PixelType::BAYER_RGGB).sizeAlignment(4096).build();
    ASSERT_EQ(layout.requiredBufferSize(), 12001280);

    /// NV12 layout, forcing width alignment to 64, height alignment to 16, and buffer size alignment to 4096
    layout = LayoutDescriptor::Builder(4000, 3000)
                     .imageLayout(ImageLayout::NV12)
                     .widthAlignment(64)
                     .heightAlignment(16)
                     .sizeAlignment(4096)
                     .build();
    ASSERT_EQ(layout.requiredBufferSize(), 18194432);
}

/// Checks slice packing and channel separation for every standard layout.
TEST(LayoutTest, TestDepthLayouts) {
    struct ExpectedLayout {
        ImageLayout imageLayout;
        int64_t lumaDepthStride;
        int64_t chromaDepthStride;
        int64_t secondChannelOffset;
        int64_t thirdChannelOffset;
        int64_t bufferSize;
    };

    const ExpectedLayout expectedLayouts[] = {{ImageLayout::PLANAR, 32, 32, 96, 192, 288},
                                              {ImageLayout::INTERLEAVED, 64, 64, 1, 2, 192},
                                              {ImageLayout::YUV_420, 32, 16, 96, 128, 160},
                                              {ImageLayout::NV12, 32, 16, 96, 97, 128}};

    for (const auto& expected : expectedLayouts) {
        SCOPED_TRACE(toString(expected.imageLayout));
        const auto layout = LayoutDescriptor::Builder(5, 3, 3)
                                    .numChannels(3)
                                    .imageLayout(expected.imageLayout)
                                    .widthAlignment(4)
                                    .heightAlignment(4)
                                    .build();

        ASSERT_EQ(layout.depth, 3);
        EXPECT_EQ(layout.channels[0].sliceStride, expected.lumaDepthStride);
        EXPECT_EQ(layout.channels[1].sliceStride, expected.chromaDepthStride);
        EXPECT_EQ(layout.channels[2].sliceStride, expected.chromaDepthStride);
        EXPECT_EQ(layout.channels[0].offset, 0);
        EXPECT_EQ(layout.channels[1].offset, expected.secondChannelOffset);
        EXPECT_EQ(layout.channels[2].offset, expected.thirdChannelOffset);
        EXPECT_EQ(layout.requiredBufferSize(), expected.bufferSize);
    }
}

/// Checks backward-compatible defaults, rebuilding, validation, and final buffer alignment.
TEST(LayoutTest, TestDepthBuilder) {
    const auto flatLayout = LayoutDescriptor::Builder(3, 3).numChannels(2).sizeAlignment(16).build();
    EXPECT_EQ(flatLayout.depth, 1);
    EXPECT_EQ(flatLayout.depthAlignment, CXXIMG_DEFAULT_DEPTH_ALIGNMENT);
    EXPECT_EQ(flatLayout.channels[1].offset, 9);
    EXPECT_EQ(flatLayout.requiredBufferSize(), 32);

    const auto volumeLayout = LayoutDescriptor::Builder(flatLayout).depth(3).build();
    EXPECT_EQ(volumeLayout.depth, 3);
    EXPECT_EQ(volumeLayout.channels[1].offset, 27);
    EXPECT_EQ(volumeLayout.requiredBufferSize(), 64); // Align the whole volume, not each slice.

    const auto copiedLayout = LayoutDescriptor::Builder(volumeLayout).build();
    EXPECT_EQ(copiedLayout.depth, 3);
    EXPECT_EQ(copiedLayout.channels[1].offset, 27);
    EXPECT_EQ(copiedLayout.requiredBufferSize(), 64);

    // Reusing a builder must recompute slice strides when height changes.
    auto builder = LayoutDescriptor::Builder(3, 3, 2).numChannels(2);
    builder.build();
    const auto tallerLayout = builder.height(4).build();
    EXPECT_EQ(tallerLayout.channels[0].sliceStride, 12);
    EXPECT_EQ(tallerLayout.channels[1].offset, 24);
    EXPECT_EQ(tallerLayout.requiredBufferSize(), 48);

    EXPECT_THROW(LayoutDescriptor::Builder(2, 2, 0).numChannels(1).build(), std::invalid_argument);
    EXPECT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).depth(-1).build(), std::invalid_argument);
}

/// Checks that depth padding changes allocation and channel starts without changing slice strides.
TEST(LayoutTest, TestDepthAlignment) {
    struct ExpectedLayout {
        ImageLayout imageLayout;
        int64_t sliceStride;
        int64_t secondChannelOffset;
        int64_t thirdChannelOffset;
        int64_t bufferSize;
    };

    const ExpectedLayout expectedLayouts[] = {{ImageLayout::PLANAR, 32, 128, 256, 384},
                                              {ImageLayout::INTERLEAVED, 64, 1, 2, 256},
                                              {ImageLayout::YUV_420, 32, 128, 192, 256},
                                              {ImageLayout::NV12, 32, 128, 129, 192},
                                              {ImageLayout::CUSTOM, 32, 128, 256, 384}};

    for (const auto& expected : expectedLayouts) {
        SCOPED_TRACE(toString(expected.imageLayout));
        const auto layout = LayoutDescriptor::Builder(5, 3, 3)
                                    .numChannels(3)
                                    .imageLayout(expected.imageLayout)
                                    .widthAlignment(4)
                                    .heightAlignment(4)
                                    .depthAlignment(4)
                                    .build();

        EXPECT_EQ(layout.depth, 3);
        EXPECT_EQ(layout.depthAlignment, 4);
        EXPECT_EQ(layout.channels[0].sliceStride, expected.sliceStride);
        EXPECT_EQ(layout.channels[1].offset, expected.secondChannelOffset);
        EXPECT_EQ(layout.channels[2].offset, expected.thirdChannelOffset);
        EXPECT_EQ(layout.requiredBufferSize(), expected.bufferSize);
    }

    EXPECT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).depthAlignment(0).build(), std::invalid_argument);
    EXPECT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).depthAlignment(-2).build(), std::invalid_argument);
    EXPECT_THROW(LayoutDescriptor::Builder(2, 2).numChannels(1).depthAlignment(3).build(), std::invalid_argument);
}

/// Checks depth padding with custom slice strides, XY borders, and final size alignment.
TEST(LayoutTest, TestCustomDepthAlignment) {
    auto builder = LayoutDescriptor::Builder(3, 2, 3)
                           .numChannels(2)
                           .imageLayout(ImageLayout::CUSTOM)
                           .border(1)
                           .channelStrides(0, 8, 1, 40)
                           .channelStrides(1, 16, 2, 80)
                           .depthAlignment(4)
                           .sizeAlignment(128);
    const auto layout = builder.build();
    EXPECT_EQ(layout.channels[0].sliceStride, 40);
    EXPECT_EQ(layout.channels[1].sliceStride, 80);
    EXPECT_EQ(layout.channels[0].offset, 9);
    EXPECT_EQ(layout.channels[1].offset, 178);
    EXPECT_EQ(layout.requiredBufferSize(), 512);

    const auto copiedLayout = LayoutDescriptor::Builder(layout).build();
    EXPECT_EQ(copiedLayout.depthAlignment, 4);
    EXPECT_EQ(copiedLayout.channels[1].offset, 178);
    EXPECT_EQ(copiedLayout.requiredBufferSize(), 512);

    const auto paddedLayout = builder.depthAlignment(8).build();
    EXPECT_EQ(paddedLayout.depth, 3);
    EXPECT_EQ(paddedLayout.channels[0].sliceStride, 40);
    EXPECT_EQ(paddedLayout.channels[1].sliceStride, 80);
    EXPECT_EQ(paddedLayout.channels[1].offset, 338);
    EXPECT_EQ(paddedLayout.requiredBufferSize(), 1024);
}

/// Checks that borders cover each XY slice without adding depth padding.
TEST(LayoutTest, TestDepthBorder) {
    const auto layout = LayoutDescriptor::Builder(3, 3, 2).numChannels(2).border(1).build();
    EXPECT_EQ(layout.channels[0].rowStride, 5);
    EXPECT_EQ(layout.channels[0].sliceStride, 25);
    EXPECT_EQ(layout.channels[0].offset, 6);
    EXPECT_EQ(layout.channels[1].offset, 56);
    EXPECT_EQ(layout.requiredBufferSize(), 100);

    const auto yuvLayout = LayoutDescriptor::Builder(3, 3, 2).imageLayout(ImageLayout::YUV_420).border(2).build();
    EXPECT_EQ(yuvLayout.channels[0].sliceStride, 64);
    EXPECT_EQ(yuvLayout.channels[1].sliceStride, 16);
    EXPECT_EQ(yuvLayout.channels[0].offset, 18);
    EXPECT_EQ(yuvLayout.channels[1].offset, 133);
    EXPECT_EQ(yuvLayout.channels[2].offset, 149);
    EXPECT_EQ(yuvLayout.requiredBufferSize(), 160);
}

/// Checks independent custom slice strides and subsampling across all spatial dimensions.
TEST(LayoutTest, TestCustomDepth) {
    const auto layout = LayoutDescriptor::Builder(5, 5, 3)
                                .numChannels(3)
                                .imageLayout(ImageLayout::CUSTOM)
                                .channelSubsample(1, 1)
                                .channelSubsample(2, 2)
                                .build();
    EXPECT_EQ(layout.channels[0].sliceStride, 64);
    EXPECT_EQ(layout.channels[1].sliceStride, 16);
    EXPECT_EQ(layout.channels[2].sliceStride, 4);
    EXPECT_EQ(layout.channels[1].offset, 192);
    EXPECT_EQ(layout.channels[2].offset, 224);
    EXPECT_EQ(layout.requiredBufferSize(), 228);

    const auto paddedLayout = LayoutDescriptor::Builder(3, 2, 3)
                                      .numChannels(2)
                                      .imageLayout(ImageLayout::CUSTOM)
                                      .channelStrides(0, 4, 1, 12)
                                      .channelStrides(1, 8, 2, 24)
                                      .build();
    EXPECT_EQ(paddedLayout.channels[0].sliceStride, 12);
    EXPECT_EQ(paddedLayout.channels[1].sliceStride, 24);
    EXPECT_EQ(paddedLayout.channels[1].offset, 36);
    EXPECT_EQ(paddedLayout.requiredBufferSize(), 108);

    const auto copiedLayout = LayoutDescriptor::Builder(paddedLayout).build();
    EXPECT_EQ(copiedLayout.channels[0].sliceStride, 12);
    EXPECT_EQ(copiedLayout.channels[1].sliceStride, 24);
    EXPECT_EQ(copiedLayout.requiredBufferSize(), 108);
}

/// Checks subsampled depth storage for odd volumes and the single-slice 2D case.
TEST(LayoutTest, TestSubsampledDepth) {
    for (const auto layoutKind : {ImageLayout::YUV_420, ImageLayout::NV12}) {
        SCOPED_TRACE(toString(layoutKind));
        const auto layout = LayoutDescriptor::Builder(4, 4, 3).imageLayout(layoutKind).build();
        EXPECT_EQ(layout.channels[1].offset, 48);
        EXPECT_EQ(layout.channels[2].offset, layoutKind == ImageLayout::YUV_420 ? 56 : 49);
        EXPECT_EQ(layout.requiredBufferSize(), 64);

        const auto flatLayout = LayoutDescriptor::Builder(layout).depth(1).build();
        EXPECT_EQ(flatLayout.channels[1].offset, 16);
        EXPECT_EQ(flatLayout.requiredBufferSize(), 24);
    }
}

/// Checks channel depth is subsampled before alignment and contributes to each channel's storage span.
TEST(LayoutTest, TestSubsampledDepthAlignment) {
    const auto layout = LayoutDescriptor::Builder(5, 5, 5)
                                .numChannels(3)
                                .imageLayout(ImageLayout::CUSTOM)
                                .channelSubsample(1, 1)
                                .channelSubsample(2, 2)
                                .border(1)
                                .depthAlignment(4)
                                .sizeAlignment(128)
                                .build();
    EXPECT_EQ(layout.channels[0].sliceStride, 64);
    EXPECT_EQ(layout.channels[1].sliceStride, 16);
    EXPECT_EQ(layout.channels[2].sliceStride, 4);
    EXPECT_EQ(layout.channels[0].offset, 9);
    EXPECT_EQ(layout.channels[1].offset, 512);
    EXPECT_EQ(layout.channels[2].offset, 576);
    EXPECT_EQ(layout.requiredBufferSize(), 640);
}

/// Checks that volume offsets and sizes use 64-bit arithmetic.
TEST(LayoutTest, TestLargeDepth) {
    const auto layout = LayoutDescriptor::Builder(65536, 65536, 2).numChannels(2).build();
    EXPECT_EQ(layout.channels[0].sliceStride, int64_t{1} << 32);
    EXPECT_EQ(layout.channels[1].offset, int64_t{1} << 33);
    EXPECT_EQ(layout.requiredBufferSize(), int64_t{1} << 34);

    const ImageDescriptor<uint8_t> descriptor(layout, nullptr);
    const auto roi = image::computeRoiDescriptor(descriptor, Rect{3, 2, 4, 2});
    const int64_t roiOffset = 2 * int64_t{65536} + 3;
    EXPECT_EQ(roi.layout.channels[0].offset, roiOffset);
    EXPECT_EQ(roi.layout.channels[1].offset, (int64_t{1} << 33) + roiOffset);
    EXPECT_EQ(roi.layout.channels[1].sliceStride, int64_t{1} << 32);
}

/// Checks that ROI views retain depth padding and the original distance between slices.
TEST(LayoutTest, TestDepthRoiDescriptor) {
    const ImageDescriptor<uint8_t> descriptor(
            LayoutDescriptor::Builder(8, 6, 3).pixelType(PixelType::GRAYSCALE).depthAlignment(4).build(), nullptr);

    const auto roi = image::computeRoiDescriptor(descriptor, Rect{2, 2, 4, 2});
    EXPECT_EQ(roi.layout.depth, 3);
    EXPECT_EQ(roi.layout.depthAlignment, 4);
    EXPECT_EQ(roi.layout.channels[0].sliceStride, 48);
    EXPECT_EQ(roi.layout.channels[0].offset, 18);
}

/// Checks Bayer channel geometry for a 2D image.
TEST(LayoutTest, TestBayerPlanarDescriptor) {
    const ImageDescriptor<uint8_t> descriptor(LayoutDescriptor::Builder(8, 6).pixelType(PixelType::BAYER_RGGB).build(),
                                              nullptr);

    const auto bayerChannels = image::computeBayerPlanarDescriptor(descriptor);
    EXPECT_EQ(bayerChannels.layout.width, 4);
    EXPECT_EQ(bayerChannels.layout.height, 3);
    EXPECT_EQ(bayerChannels.layout.depth, 1);
    EXPECT_EQ(bayerChannels.layout.numChannels, 4);
    EXPECT_EQ(bayerChannels.layout.channels[0].offset, 0);
    EXPECT_EQ(bayerChannels.layout.channels[1].offset, 1);
    EXPECT_EQ(bayerChannels.layout.channels[2].offset, 8);
    EXPECT_EQ(bayerChannels.layout.channels[3].offset, 9);

    for (int i = 0; i < bayerChannels.layout.numChannels; ++i) {
        EXPECT_EQ(bayerChannels.layout.channels[i].rowStride, 16);
        EXPECT_EQ(bayerChannels.layout.channels[i].pixelStride, 2);
        EXPECT_EQ(bayerChannels.layout.channels[i].sliceStride, 48);
    }
}

#ifdef CXXIMG_HAVE_HALIDE
/// Checks that Halide volumes omit the channel dimension when only one channel is present.
TEST(LayoutTest, TestHalideDepth) {
    const ImageDescriptor<uint8_t> descriptor(LayoutDescriptor::Builder(5, 3, 2).numChannels(3).build(), nullptr);
    EXPECT_EQ(descriptor.halide->buffer.dimensions, 4);
    EXPECT_EQ(descriptor.halide->dim[2].extent, 3);
    EXPECT_EQ(descriptor.halide->dim[2].stride, 30);
    EXPECT_EQ(descriptor.halide->dim[3].extent, 2);
    EXPECT_EQ(descriptor.halide->dim[3].stride, 15);

    const ImageDescriptor<uint8_t> singleChannelDescriptor(
            LayoutDescriptor::Builder(5, 3, 2).numChannels(1).depthAlignment(4).build(), nullptr);
    EXPECT_EQ(singleChannelDescriptor.halide->buffer.dimensions, 3);
    EXPECT_EQ(singleChannelDescriptor.halide->dim[2].extent, 2);
    EXPECT_EQ(singleChannelDescriptor.halide->dim[2].stride, 15);
}
#endif
