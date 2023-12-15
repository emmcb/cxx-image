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

/// @addtogroup math
/// @{

/// Simple 2D coordinates struct.
struct Point2 final {
    float x; ///< X Coordinate.
    float y; ///< Y Coordinate.
};

/// @}

/// Element-wise addition.
/// @relates Point2
inline Point2 operator+(const Point2 &lhs, const Point2 &rhs) {
    return Point2{lhs.x + rhs.x, lhs.y + rhs.y};
}

/// Element-wise subtraction.
/// @relates Point2
inline Point2 operator-(const Point2 &lhs, const Point2 &rhs) {
    return Point2{lhs.x - rhs.x, lhs.y - rhs.y};
}

/// Scalar multiplication.
/// @relates Point2
inline Point2 operator*(float lhs, const Point2 &rhs) {
    return Point2{lhs * rhs.x, lhs * rhs.y};
}

/// Scalar division.
/// @relates Point2
inline Point2 operator/(const Point2 &lhs, float rhs) {
    return Point2{lhs.x / rhs, lhs.x / rhs};
}

/// Geometry functions
/// @ingroup math
namespace geometry {

/// Returns the dot product between two points.
inline float dot(const Point2 &a, const Point2 &b) {
    return a.x * b.x + a.y * b.y;
}

/// Returns the squared distance between two points.
inline float squaredDistance(const Point2 &a, const Point2 &b) {
    const Point2 p = a - b;
    return dot(p, p);
}

/// Returns the distance between two points.
inline float distance(const Point2 &a, const Point2 &b) {
    return std::sqrt(squaredDistance(a, b));
}

/// Returns the squared length (L2 norm) of the given point.
inline float squaredLength(const Point2 &pt) {
    return dot(pt, pt);
}

/// Returns the length (L2 norm) of the given point.
inline float length(const Point2 &pt) {
    return std::sqrt(squaredLength(pt));
}

/// Normalizes the given point.
inline Point2 normalize(const Point2 &pt) {
    return pt / length(pt);
}

/// Projects a point onto the given line.
inline Point2 pointLineProjection(const Point2 &pt, float slope, float intercept) {
    const float perpSlope = -1.0f / slope;
    const float perpIntercept = pt.y - perpSlope * pt.x;
    const float intersectX = (perpIntercept - intercept) / (slope - perpSlope);
    return Point2{intersectX, slope * intersectX + intercept};
}

/// Projects a point onto the given [AB] segment.
inline Point2 pointSegmentProjection(const Point2 &pt, const Point2 &a, const Point2 &b) {
    const float t = math::saturate(dot(pt - a, b - a) / squaredDistance(a, b), 0.0f, 1.0f);
    return Point2{a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
}

} // namespace geometry

} // namespace cxximg
