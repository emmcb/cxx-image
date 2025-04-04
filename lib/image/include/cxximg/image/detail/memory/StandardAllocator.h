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

#include "cxximg/image/detail/memory/Allocator.h"

#include <new>

namespace cxximg {

namespace memory {

namespace detail {

/// Standard allocator that directly uses operator new/delete.
class StandardAllocator : public Allocator {
public:
    void *allocate(int64_t size) override { return ::operator new[](size, std::align_val_t(CXXIMG_BASE_ALIGNMENT)); }

    void deallocate(void *ptr, [[maybe_unused]] int64_t size) override {
        ::operator delete[](ptr, std::align_val_t(CXXIMG_BASE_ALIGNMENT));
    }

    /// Get the singleton instance.
    static StandardAllocator &instance() {
        static StandardAllocator sAllocator;
        return sAllocator;
    }
};

} // namespace detail

} // namespace memory

} // namespace cxximg
