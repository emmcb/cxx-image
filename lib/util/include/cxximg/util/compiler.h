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

#if __has_attribute(always_inline)
#define UTIL_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER) && (_MSC_VER >= 1927)
#define UTIL_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define UTIL_ALWAYS_INLINE
#endif
