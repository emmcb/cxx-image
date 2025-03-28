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

#include "cxximg/image/Allocation.h"
#include "cxximg/image/Image.h"

#include <gtest/gtest.h>

using namespace cxximg;

TEST(ImageAllocatorTest, StandardAllocator) {
    // Set standard allocator (the default one)
    memory::useStandardAllocator();

    // Create and destroy an image
    Image8u imageA(LayoutDescriptor::Builder(100, 100).numPlanes(3).build());

    // Create another image (memory should be newly allocated)
    Image8u imageB(LayoutDescriptor::Builder(100, 100).numPlanes(3).build());

    // Pointers should be different
    EXPECT_NE(imageA.data(), imageB.data());
}

TEST(ImageAllocatorTest, RecyclingAllocator) {
    // Set recycling allocator
    memory::useRecyclingAllocator();

    // Clear the allocator cache
    memory::clearAllocatorCache();

    // Create and destroy an image
    uint8_t *ptrA = nullptr;
    {
        Image8u imageA(LayoutDescriptor::Builder(100, 100).numPlanes(3).build());
        ptrA = imageA.data();
    }

    // Create another image with the same size (memory should be reused)
    Image8u imageB(LayoutDescriptor::Builder(100, 100).numPlanes(3).build());

    // Pointers should be the same
    EXPECT_EQ(ptrA, imageB.data());
}

TEST(ImageAllocatorTest, RecyclingAllocatorDifferentSizes) {
    // Set recycling allocator
    memory::useRecyclingAllocator();

    // Clear the allocator cache
    memory::clearAllocatorCache();

    // Create and destroy an image
    uint8_t *ptrA = nullptr;
    {
        Image8u imageA(LayoutDescriptor::Builder(100, 100).numPlanes(3).build());
        ptrA = imageA.data();
    }

    // Create another image with the different size
    Image8u imageB(LayoutDescriptor::Builder(200, 200).numPlanes(3).build());

    // Pointers should be different
    EXPECT_NE(ptrA, imageB.data());
}
