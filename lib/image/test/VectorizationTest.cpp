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

#include "cxximg/image/Image.h"
#include "cxximg/image/view/PixelView.h"
#include "cxximg/math/Matrix.h"

#include <cstdint>

using namespace cxximg;
using Value = CXXIMG_VECTORIZE_VALUE;

// Compile one probe per translation unit so diagnostics belong unambiguously to its case.
#if CXXIMG_VECTORIZE_CASE == 1

/// Checks affine channel expressions evaluated over XYZ coordinates.
void channelAffine(ChannelView<Value>& destination, const ChannelView<Value>& source) {
    destination = source * Value(2) + Value(3);
}

#elif CXXIMG_VECTORIZE_CASE == 2

/// Checks image expressions with runtime channel layouts and singleton-depth broadcasting.
void imageBroadcast(ImageView<Value>& destination, const ImageView<Value>& source, const ImageView<Value>& flat) {
    destination = source * Value(2) + flat;
}

#elif CXXIMG_VECTORIZE_CASE == 3

/// Checks channel selection from constant pixels in an image expression.
void pixelConstants(ImageView<Value>& destination, const ImageView<Value>& source) {
    destination = source * Pixel<Value, 3>(Value(2)) + Pixel<Value, 3>(Value(3));
}

#elif CXXIMG_VECTORIZE_CASE == 4

/// Checks whole-pixel matrix transforms evaluated over XYZ coordinates.
void matrixPixels(PixelView<Value, 3>& destination, const PixelView<Value, 3>& source, const Matrix<3, 3>& matrix) {
    destination = matrix * source;
}

#elif CXXIMG_VECTORIZE_CASE == 5

/// Checks whole-pixel expressions evaluated over XYZ coordinates.
void wholePixels(PixelView<Value, 3>& destination, const PixelView<Value, 3>& source) {
    destination = source * Value(2) + Pixel<Value, 3>(Value(3));
}

#elif CXXIMG_VECTORIZE_CASE == 6

/// Supplies an opaque callable whose unknown side effects prevent loop vectorization.
Value opaqueSample(int x, int y, int z) noexcept;

/// Checks diagnostics distinguish a scalar loop containing an opaque function call.
void opaqueCallable(ChannelView<Value>& destination) {
    destination = opaqueSample;
}

#else
#error "Unknown vectorization case"
#endif
