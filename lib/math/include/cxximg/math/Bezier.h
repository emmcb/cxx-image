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

#include "cxximg/math/Point.h"

#include <cmath>

namespace cxximg {

namespace math {

/// Quadratic bezier curve, parametrized by t in [0,1].
inline constexpr float bezierQuadratic(float p0, float p1, float p2, float t) {
    const float u = 1.0f - t;
    return p0 * u * u + p1 * u * t * 2.0f + p2 * t * t;
}

/// Cubic bezier curve, parametrized by t in [0,1].
inline constexpr float bezierCubic(float p0, float p1, float p2, float p3, float t) {
    const float u = 1.0f - t;
    return p0 * u * u * u + p1 * u * u * t * 3.0f + p2 * u * t * t * 3.0f + p3 * t * t * t;
}

/// Quartic bezier curve, parametrized by t in [0,1].
inline constexpr float bezierQuartic(float p0, float p1, float p2, float p3, float p4, float t) {
    const float u = 1.0f - t;
    return p0 * u * u * u * u + p1 * u * u * u * t * 4.0f + p2 * u * u * t * t * 6.0f + p3 * u * t * t * t * 4.0f +
           p4 * t * t * t * t;
}

} // namespace math

/// @addtogroup math
/// @{

/// Bezier curve of order 2.
struct QuadraticBezier final {
    Point2 p0;
    Point2 p1;
    Point2 p2;

    /// First point of the curve.
    const Point2 &front() const { return p0; }
    /// Last point of the curve.
    const Point2 &back() const { return p2; }

    /// Evaluate curve X coordinate at parameter t in [0, 1].
    float evaluateX(float t) const { return math::bezierQuadratic(p0.x, p1.x, p2.x, t); }

    /// Evaluate curve Y coordinate at parameter t in [0, 1].
    float evaluateY(float t) const { return math::bezierQuadratic(p0.y, p1.y, p2.y, t); }

    /// Evaluate curve at parameter t in [0, 1].
    Point2 evaluate(float t) const { return {evaluateX(t), evaluateY(t)}; }

    /// Evaluate curve Y derivative with respect to X at parameter t in [0, 1].
    float evaluateDerivative(float t) const {
        const float halfDerivativeX = (1.0f - t) * (p1.x - p0.x) + t * (p2.x - p1.x);
        const float halfDerivativeY = (1.0f - t) * (p1.y - p0.y) + t * (p2.y - p1.y);

        return halfDerivativeY / halfDerivativeX;
    }

    /// Compute the parameter t in [0, 1] at the given X coordinate.
    float parameterAt(float x) const {
        // std::numeric_limits<float>::epsilon() leads to floating point inaccuracies in the calculations below, so
        // choose a slightly higher epsilon value.
        constexpr float EPS = 1e-4f;

        if (x <= p0.x) {
            return 0.0f;
        }
        if (x >= p2.x) {
            return 1.0f;
        }

        // A quadratic Bezier curve is a parabola, we can find t by solving the corresponding 2nd order equation.
        const float denom = p0.x - 2.0f * p1.x + p2.x;
        if (std::abs(denom) < EPS) {
            if (std::abs(p1.x - p0.x) < EPS) {
                return 0.0f; // p0.x == p1.x == p2.x
            }

            return 0.5f * (x - p0.x) / (p1.x - p0.x); // p0.x, p1.x and p2.x all lie on the same line
        }

        const float deltaSquared = p1.x * p1.x + x * denom - p0.x * p2.x;
        return (p0.x - p1.x + std::sqrt(deltaSquared)) / denom;
    }

    /// Compute the Y coordinate at the given X coordinate.
    float at(float x) const { return evaluateY(parameterAt(x)); }

    /// Compute the Y derivative at the given X coordinate.
    float derivativeAt(float x) const { return evaluateDerivative(parameterAt(x)); }
};

/// Bezier curve of order 3.
struct CubicBezier final {
    Point2 p0;
    Point2 p1;
    Point2 p2;
    Point2 p3;

    /// First point of the curve.
    const Point2 &front() const { return p0; }
    /// Last point of the curve.
    const Point2 &back() const { return p3; }

    /// Evaluate curve X coordinate at parameter t in [0, 1].
    float evaluateX(float t) const { return math::bezierCubic(p0.x, p1.x, p2.x, p3.x, t); }

    /// Evaluate curve Y coordinate at parameter t in [0, 1].
    float evaluateY(float t) const { return math::bezierCubic(p0.y, p1.y, p2.y, p3.y, t); }

    /// Evaluate curve at parameter t in [0, 1].
    Point2 evaluate(float t) const { return {evaluateX(t), evaluateY(t)}; }
};

/// Bezier curve of order 4.
struct QuarticBezier final {
    Point2 p0;
    Point2 p1;
    Point2 p2;
    Point2 p3;
    Point2 p4;

    /// First point of the curve.
    const Point2 &front() const { return p0; }
    /// Last point of the curve.
    const Point2 &back() const { return p4; }

    /// Evaluate curve X coordinate at parameter t in [0, 1].
    float evaluateX(float t) const { return math::bezierQuartic(p0.x, p1.x, p2.x, p3.x, p4.x, t); }

    /// Evaluate curve Y coordinate at parameter t in [0, 1].
    float evaluateY(float t) const { return math::bezierQuartic(p0.y, p1.y, p2.y, p3.y, p4.y, t); }

    /// Evaluate curve at parameter t in [0, 1].
    Point2 evaluate(float t) const { return {evaluateX(t), evaluateY(t)}; }
};

/// @}

} // namespace cxximg
