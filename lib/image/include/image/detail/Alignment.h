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

#pragma once

#include "math/math.h"

namespace cxximg {

namespace detail {

/// Align dimension depending on alignement value.
/// @param size dimension to be aligned.
/// @param sizeAlignment alignement value (shall be a power of 2).
/// @return aligned size dimension.
template <typename T>
T alignDimension(T size, int sizeAlignment) {
    return math::roundUp(size, sizeAlignment);
}

/// Align dimension depending on current plane subsampling value.
/// @param size dimension to be aligned.
/// @param sizeAlignment alignement value (shall be a power of 2).
/// @param subsample subsample of current plane.
/// @param maxSubsample maximum subsample value defined in the image planes.
/// @return aligned size dimension.
inline int alignDimension(int size, int sizeAlignment, int subsample, int maxSubsample) {
    return alignDimension(alignDimension(size, 1 << maxSubsample) >> subsample, sizeAlignment);
}

} // namespace detail

} // namespace cxximg
