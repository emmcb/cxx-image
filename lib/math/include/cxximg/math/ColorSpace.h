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

#include "cxximg/math/Matrix.h"
#include "cxximg/math/Pixel.h"

#include <cmath>
#include <optional>
#include <string>

namespace cxximg {

/// @addtogroup math
/// @{

/// RGB color spaces.
enum class RgbColorSpace {
    ADOBE_RGB,  ///< Adobe RGB (1998).
    DISPLAY_P3, ///< P3-D65 (Display)
    REC2020,    ///< Wide gamut color space also kwnown as Bt.2020.
    SRGB,       ///< Standard RGB.
    XYZ_D50,    ///< CIE XYZ with D50 white point.
    XYZ_D65     ///< CIE XYZ with D65 white point.
};

/// RGB transfer functions.
enum class RgbTransferFunction {
    GAMMA22, ///< Gamma 2.2 transfer function.
    LINEAR,  ///< Linear transfer function.
    SRGB     ///< sRGB transfer function.
};

inline const char *toString(RgbColorSpace colorSpace) {
    switch (colorSpace) {
        case RgbColorSpace::ADOBE_RGB:
            return "adobe_rgb";
        case RgbColorSpace::DISPLAY_P3:
            return "display_p3";
        case RgbColorSpace::REC2020:
            return "rec2020";
        case RgbColorSpace::SRGB:
            return "srgb";
        case RgbColorSpace::XYZ_D50:
            return "xyz_d50";
        case RgbColorSpace::XYZ_D65:
            return "xyz_d65";
    }
    return "undefined";
}

inline const char *toString(RgbTransferFunction transferFunction) {
    switch (transferFunction) {
        case RgbTransferFunction::GAMMA22:
            return "gamma22";
        case RgbTransferFunction::LINEAR:
            return "linear";
        case RgbTransferFunction::SRGB:
            return "srgb";
    }
    return "undefined";
}

inline std::optional<RgbColorSpace> parseRgbColorSpace(const std::string &colorSpace) {
    if (colorSpace == "adobe_rgb") {
        return RgbColorSpace::ADOBE_RGB;
    }
    if (colorSpace == "display_p3") {
        return RgbColorSpace::DISPLAY_P3;
    }
    if (colorSpace == "rec2020") {
        return RgbColorSpace::REC2020;
    }
    if (colorSpace == "srgb") {
        return RgbColorSpace::SRGB;
    }
    if (colorSpace == "xyz_d50") {
        return RgbColorSpace::XYZ_D50;
    }
    if (colorSpace == "xyz_d65") {
        return RgbColorSpace::XYZ_D65;
    }
    return std::nullopt;
}

inline std::optional<RgbTransferFunction> parseRgbTransferFunction(const std::string &transferFunction) {
    if (transferFunction == "gamma22") {
        return RgbTransferFunction::GAMMA22;
    }
    if (transferFunction == "linear") {
        return RgbTransferFunction::LINEAR;
    }
    if (transferFunction == "srgb") {
        return RgbTransferFunction::SRGB;
    }
    return std::nullopt;
}

/// Color space functions
namespace colorspace {

static constexpr Pixel3f D50_WHITE_XYZ{0.96422f, 1.0f, 0.82521f};
static constexpr Pixel3f D65_WHITE_XYZ{0.95047f, 1.0f, 1.08883f};

namespace detail {

// These matrices have been computed using http://www.russellcottrell.com/photo/matrixCalculator.htm
// The connection space is XYZ with D65 white point.

// XYZ D65 <--> Adobe RGB

static constexpr Matrix3 ADOBE_RGB_TO_XYZ_D65 = {{0.5766690, 0.1855582, 0.1882286},
                                                 {0.2973450, 0.6273636, 0.0752915},
                                                 {0.0270314, 0.0706889, 0.9913375}};

static constexpr Matrix3 XYZ_D65_TO_ADOBE_RGB = {{2.0415879, -0.5650070, -0.3447314},
                                                 {-0.9692436, 1.8759675, 0.0415551},
                                                 {0.0134443, -0.1183624, 1.0151750}};

// XYZ D65 <--> Display P3

static constexpr Matrix3 DISPLAY_P3_TO_XYZ_D65 = {{0.4865709, 0.2656677, 0.1982173},
                                                  {0.2289746, 0.6917385, 0.0792869},
                                                  {0.0000000, 0.0451134, 1.0439444}};

static constexpr Matrix3 XYZ_D65_TO_DISPLAY_P3 = {{2.4934969, -0.9313836, -0.4027108},
                                                  {-0.8294890, 1.7626641, 0.0236247},
                                                  {0.0358458, -0.0761724, 0.9568845}};

// XYZ D65 <--> Rec. 2020

static constexpr Matrix3 REC2020_TO_XYZ_D65 = {{0.6369580, 0.1446169, 0.1688810},
                                               {0.2627002, 0.6779981, 0.0593017},
                                               {0.0000000, 0.0280727, 1.0609851}};

static constexpr Matrix3 XYZ_D65_TO_REC2020 = {{1.7166512, -0.3556708, -0.2533663},
                                               {-0.6666844, 1.6164812, 0.0157685},
                                               {0.0176399, -0.0427706, 0.9421031}};

// XYZ D65 <--> sRGB

static constexpr Matrix3 SRGB_TO_XYZ_D65 = {{0.4123908, 0.3575843, 0.1804808},
                                            {0.2126390, 0.7151687, 0.0721923},
                                            {0.0193308, 0.1191948, 0.9505322}};

static constexpr Matrix3 XYZ_D65_TO_SRGB = {{3.2409699, -1.5373832, -0.4986108},
                                            {-0.9692436, 1.8759675, 0.0415551},
                                            {0.0556301, -0.2039770, 1.0569715}};

// XYZ D65 <--> XYZ D50

static constexpr Matrix3 XYZ_D50_TO_XYZ_D65 = {{0.9555766, -0.0230393, 0.0631636},
                                               {-0.0282895, 1.0099416, 0.0210077},
                                               {0.0122982, -0.0204830, 1.3299098}};

static constexpr Matrix3 XYZ_D65_TO_XYZ_D50 = {{1.0478112, 0.0228866, -0.0501270},
                                               {0.0295424, 0.9904844, -0.0170491},
                                               {-0.0092345, 0.0150436, 0.7521316}};

} // namespace detail

/// Compute the linear Bradford adaptation matrix to convert from an illuminant to another.
inline Matrix3 linearBradfordAdaptation(const Pixel3f &srcWhiteXYZ, const Pixel3f &dstWhiteXYZ) {
    // Use the linearized Bradford adaptation matrix.
    constexpr Matrix3 M_A{{0.8951000f, 0.2664000f, -0.1614000f},
                          {-0.7502000f, 1.7135000f, 0.0367000f},
                          {0.0389000f, -0.0685000f, 1.0296000f}};

    constexpr Matrix3 M_A_INV{{0.9869929f, -0.1470543f, 0.1599627f},
                              {0.4323053f, 0.5183603f, 0.0492912f},
                              {-0.0085287f, 0.0400428f, 0.9684867f}};

    const Pixel3f srcWhiteLMS = M_A * srcWhiteXYZ;
    const Pixel3f dstWhiteLMS = M_A * dstWhiteXYZ;

    Matrix3 mat(0.0f);
    mat(0, 0) = dstWhiteLMS[0] / srcWhiteLMS[0];
    mat(1, 1) = dstWhiteLMS[1] / srcWhiteLMS[1];
    mat(2, 2) = dstWhiteLMS[2] / srcWhiteLMS[2];

    return M_A_INV * mat * M_A;
}

/// Compute the transformation matrix to convert from a color space to another.
inline Matrix3 transformationMatrix(RgbColorSpace from, RgbColorSpace to) {
    if (from == to) {
        return Matrix3::IDENTITY;
    }

    Matrix3 fromMatrix = [&]() {
        switch (from) {
            case RgbColorSpace::ADOBE_RGB:
                return detail::ADOBE_RGB_TO_XYZ_D65;
            case RgbColorSpace::DISPLAY_P3:
                return detail::DISPLAY_P3_TO_XYZ_D65;
            case RgbColorSpace::REC2020:
                return detail::REC2020_TO_XYZ_D65;
            case RgbColorSpace::SRGB:
                return detail::SRGB_TO_XYZ_D65;
            case RgbColorSpace::XYZ_D50:
                return detail::XYZ_D50_TO_XYZ_D65;
            case RgbColorSpace::XYZ_D65:
                return Matrix3::IDENTITY;
        }

        return Matrix3::IDENTITY; // not reachable
    }();

    Matrix3 toMatrix = [&]() {
        switch (to) {
            case RgbColorSpace::ADOBE_RGB:
                return detail::XYZ_D65_TO_ADOBE_RGB;
            case RgbColorSpace::DISPLAY_P3:
                return detail::XYZ_D65_TO_DISPLAY_P3;
            case RgbColorSpace::REC2020:
                return detail::XYZ_D65_TO_REC2020;
            case RgbColorSpace::SRGB:
                return detail::XYZ_D65_TO_SRGB;
            case RgbColorSpace::XYZ_D50:
                return detail::XYZ_D65_TO_XYZ_D50;
            case RgbColorSpace::XYZ_D65:
                return Matrix3::IDENTITY;
        }

        return Matrix3::IDENTITY; // not reachable
    }();

    return toMatrix * fromMatrix;
}

/// Apply the sRGB OETF on value x in [0, 1].
inline float srgbOetf(float x) {
    if (x <= 0.0031308f) {
        return 12.92f * x;
    }

    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

/// Apply the sRGB EOTF on value x in [0, 1].
inline float srgbEotf(float x) {
    if (x <= 0.04045f) {
        return x / 12.92f;
    }

    return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

/// Apply RGB encoding function on value x in [0, 1].
inline float encodingFunction(float x, RgbTransferFunction transferFunction) {
    switch (transferFunction) {
        case RgbTransferFunction::LINEAR:
            return x;
        case RgbTransferFunction::GAMMA22:
            return std::pow(x, 1.0f / 2.2f);
        case RgbTransferFunction::SRGB:
            return srgbOetf(x);
    }

    return x; // not reachable
}

/// Apply RGB decoding function on value x in [0, 1].
inline float decodingFunction(float x, RgbTransferFunction transferFunction) {
    switch (transferFunction) {
        case RgbTransferFunction::LINEAR:
            return x;
        case RgbTransferFunction::GAMMA22:
            return std::pow(x, 2.2f);
        case RgbTransferFunction::SRGB:
            return srgbEotf(x);
    }

    return x; // not reachable
}

} // namespace colorspace

/// @}

} // namespace cxximg
