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

#include "cxximg/math/Point.h"
#include "cxximg/math/math.h"

#include <array>

namespace cxximg {

/// @addtogroup math
/// @{

/// A curve that has been subdivided in NUM_SEGMENTS segments
template <int NUM_SEGMENTS>
class SubdividedCurve final {
public:
    SubdividedCurve() = default;

    /// Constructs from a curve to be subdivided.
    template <typename Curve>
    explicit SubdividedCurve(const Curve &curve) {
        subdivide(curve);
    }

    /// Subdivide curve in NUM_SEGMENTS segments of equal parameter t.
    template <typename Curve>
    void subdivide(const Curve &curve) {
        for (int i = 0; i < NUM_SEGMENTS + 1; ++i) {
            const float t = static_cast<float>(i) / NUM_SEGMENTS;
            mPoints[i] = curve.evaluate(t);
        }
    }

    /// First point of the curve.
    const Point2 &front() const { return mPoints.front(); }
    /// Last point of the curve.
    const Point2 &back() const { return mPoints.back(); }

    /// Interpolates the Y coordinate at the given X coordinate.
    float interpolate(float x) const {
        const auto &first = mPoints.front();
        if (x <= first.x) {
            return first.y;
        }

        for (int i = 1; i <= NUM_SEGMENTS; ++i) {
            const auto &prev = mPoints[i - 1];
            const auto &cur = mPoints[i];

            if (x > prev.x && x <= cur.x) {
                const float k = (x - prev.x) / (cur.x - prev.x);
                return math::lerp(prev.y, cur.y, k);
            }
        }

        const auto &last = mPoints.back();
        return last.y;
    }

private:
    std::array<Point2, NUM_SEGMENTS + 1> mPoints = {};
};

/// @}

} // namespace cxximg
