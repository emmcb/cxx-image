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

#include "cxximg/math/math.h"

#include <algorithm>
#include <vector>

namespace cxximg {

namespace math {

template <typename T, typename U = T>
std::vector<U> resizeLut(const std::vector<T>& lut, int newSize) {
    std::vector<U> interpolated(newSize);

    if (static_cast<int>(lut.size()) == newSize) {
        std::copy_n(lut.begin(), newSize, interpolated.begin());
    } else {
        const int xMax = lut.size() - 1;
        const float scale = static_cast<float>(xMax) / (newSize - 1);

        for (int i = 0; i < newSize; ++i) {
            const float x = scale * i;
            int x1 = static_cast<int>(x);
            int x2 = (x1 < xMax) ? x1 + 1 : x1;

            const float y = math::lerp(lut[x1], lut[x2], x - x1);
            if (std::is_floating_point_v<U>) {
                interpolated[i] = y;
            } else {
                interpolated[i] = static_cast<U>(std::lround(y));
            }
        }
    }

    return interpolated;
}

} // namespace math

} // namespace cxximg
