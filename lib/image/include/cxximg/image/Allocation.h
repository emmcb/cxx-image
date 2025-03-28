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

#pragma once

#include "cxximg/image/detail/memory/AllocatorManager.h"
#include "cxximg/image/detail/memory/RecyclingAllocator.h"
#include "cxximg/image/detail/memory/StandardAllocator.h"

namespace cxximg {

namespace memory {

/// Sets the standard allocator as the current global image allocator.
/// This is the default allocator, which immediately frees memory when images are destroyed.
inline void useStandardAllocator() {
    detail::AllocatorManager::setCurrent(detail::StandardAllocator::instance());
}

/// Sets the recycling allocator as the current global image allocator.
/// This allocator keeps freed memory around for reuse in future image allocations.
inline void useRecyclingAllocator() {
    detail::AllocatorManager::setCurrent(detail::RecyclingAllocator::instance());
}

/// Clears all memory cached by the recycling allocator.
inline void clearAllocatorCache() {
    if (auto *recyclingAllocator = dynamic_cast<detail::RecyclingAllocator *>(&detail::AllocatorManager::current())) {
        recyclingAllocator->clear();
    }
}

} // namespace memory

} // namespace cxximg
