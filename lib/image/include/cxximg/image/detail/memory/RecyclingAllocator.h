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

#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>

namespace cxximg {

namespace memory {

namespace detail {

/// Recycling allocator that keeps memory around for reuse.
class RecyclingAllocator : public Allocator {
public:
    ~RecyclingAllocator() override {
        // Clear the pool on destruction, freeing all remaining blocks
        clear();
    }

    void *allocate(int64_t size) override {
        if (size == 0) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mPool.find(size);

        if (it != mPool.end() && !it->second.empty()) {
            // Found a suitable block in the pool
            void *ptr = it->second.back();
            it->second.pop_back();

            return ptr;
        }

        // No suitable block found, allocate new using aligned new
        return ::operator new[](size, std::align_val_t(CXXIMG_BASE_ALIGNMENT));
    }

    void deallocate(void *ptr, int64_t size) override {
        if (ptr == nullptr || size == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(mMutex);
        mPool[size].push_back(ptr);
    }

    /// Clear all cached memory blocks.
    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        for (const auto &[size, blocks] : mPool) {
            for (void *block : blocks) {
                ::operator delete[](block, std::align_val_t(CXXIMG_BASE_ALIGNMENT));
            }
        }
        mPool.clear();
    }

    /// Get the singleton instance.
    static RecyclingAllocator &instance() {
        static RecyclingAllocator sAllocator;
        return sAllocator;
    }

private:
    std::unordered_map<int64_t, std::vector<void *>> mPool;
    std::mutex mMutex; // Protects access to mPool
};

} // namespace detail

} // namespace memory

} // namespace cxximg
