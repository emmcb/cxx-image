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

#include <cassert>
#include <cmath>
#include <type_traits>

namespace cxximg {

/// @defgroup math Math library

/// Math functions
/// @ingroup math
namespace math {

/// Clamps given value between min and max.
template <typename T>
constexpr T saturate(T value, T min, T max) {
    return (value < min) ? min : ((value > max) ? max : value);
}

/// Returns the linear interpolation between p1 and p2 at position t, where t is in [0,1].
inline constexpr float lerp(float p1, float p2, float t) {
    return p1 + t * (p2 - p1);
}

/// Returns the cubic interpolation (using Catmull-Rom spline) between p1 and p2 at position t, where t is in [0,1].
inline constexpr float catmullRom(float p0, float p1, float p2, float p3, float t) {
    const float a = -p0 + 3.0f * (p1 - p2) + p3;
    const float b = 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3;
    const float c = -p0 + p2;
    const float d = p1;

    return ((a * t + b) * t + c) * 0.5f * t + d;
}

/// Returns the bilinear interpolation between p11, p21, p12, p22 at position (tx, ty), where tx and ty are in [0,1].
inline constexpr float bilinearInterpolation(float p11, float p21, float p12, float p22, float tx, float ty) {
    return lerp(lerp(p11, p21, tx), lerp(p12, p22, tx), ty);
}

/// Returns the bicubic interpolation between p11, p21, p12, p22 at position (tx, ty), where tx and ty are in [0,1].
inline constexpr float bicubicInterpolation(float p00,
                                            float p10,
                                            float p20,
                                            float p30,
                                            float p01,
                                            float p11,
                                            float p21,
                                            float p31,
                                            float p02,
                                            float p12,
                                            float p22,
                                            float p32,
                                            float p03,
                                            float p13,
                                            float p23,
                                            float p33,
                                            float tx,
                                            float ty) {
    return catmullRom(catmullRom(p00, p10, p20, p30, tx),
                      catmullRom(p01, p11, p21, p31, tx),
                      catmullRom(p02, p12, p22, p32, tx),
                      catmullRom(p03, p13, p23, p33, tx),
                      ty);
}

/// Compute the ax^3 + bx^2 + cx + d cubic polynomial coefficients that cross (x1, y1) and (x2, y2) with slope m1 at
/// (x1, y1) and m2 at (x2, y2).
inline constexpr void cubicFit2Points2Slopes(float x1,
                                             float y1,
                                             float x2,
                                             float y2,
                                             float m1,
                                             float m2,
                                             float &a,
                                             float &b,
                                             float &c,
                                             float &d) {
    a = (m2 + m1 - 2.0f * (y2 - y1) / (x2 - x1)) / ((x2 - x1) * (x2 - x1));
    b = (m2 - m1) / (2.0f * (x2 - x1)) - (3.0f / 2.0f) * (x1 + x2) * a;
    c = m1 - 3.0f * x1 * x1 * a - 2 * x1 * b;
    d = y1 - x1 * x1 * x1 * a - x1 * x1 * b - x1 * c;
}

/// Division with rounding.
template <typename T>
constexpr T roundDivision(T q, T r) {
    static_assert(std::is_integral_v<T>);
    return (q + r / 2) / r;
}

/// Division with ceiling.
template <typename T>
constexpr T ceilDivision(T q, T r) {
    static_assert(std::is_integral_v<T>);
    return (q + r - 1) / r;
}

/// Check if given value is a power of 2
inline constexpr bool isPowerOf2(int value) {
    return (value != 0 && ((value & (value - 1)) == 0));
}

/// Round a number 'numToRound' by the greater value being multiple of 'multiple' (where 'multiple' is power of 2)
template <typename T>
constexpr T roundUp(T numToRound, int multiple) {
    static_assert(std::is_integral_v<T>);
    assert(isPowerOf2(multiple));
    return (numToRound + multiple - 1) & -multiple;
}

/// Return gaussian of x for a given sigma
inline float gaussian(float x, float sigma) {
    return expf(-x * x / (2.0f * sigma * sigma));
}

/// Return normalized gaussian of x for a given sigma
inline float normalizedGaussian(float x, float sigma) {
    return gaussian(x, sigma) / (sigma * sqrtf(2.0f * static_cast<float>(M_PI)));
}

/// Return the sign of the given number (-1, 0, or 1).
template <typename T>
constexpr int sign(T x, [[maybe_unused]] std::false_type isSigned) {
    return T(0) < x;
}

/// Return the sign of the given number (-1, 0, or 1).
template <typename T>
constexpr int sign(T x, [[maybe_unused]] std::true_type isSigned) {
    return (T(0) < x) - (x < T(0));
}

/// Return the sign of the given number (-1, 0, or 1).
template <typename T>
constexpr int sign(T x) {
    return sign(x, std::is_signed<T>());
}

} // namespace math

} // namespace cxximg
