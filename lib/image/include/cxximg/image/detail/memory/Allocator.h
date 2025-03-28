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

#include <cstdint>

namespace cxximg {

namespace memory {

namespace detail {

/// Base class for image allocators.
class Allocator {
public:
    virtual ~Allocator() = default;

    /// Allocate memory for image of size bytes.
    virtual void *allocate(int64_t size) = 0;

    /// Deallocate memory previously allocated.
    virtual void deallocate(void *ptr, int64_t size) = 0;
};

} // namespace detail

} // namespace memory

} // namespace cxximg
