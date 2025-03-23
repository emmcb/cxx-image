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

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cxximg {

template <typename T, int N>
class Pixel;

/// @addtogroup math
/// @{

/// Matrix with static MxN size.
template <int M, int N>
class Matrix final {
public:
    /// Identity matrix.
    static const Matrix<M, N> IDENTITY;

    /// Constructs empty matrix.
    Matrix() = default;

    /// Constructs value-initialized matrix with specified dimensions.
    explicit Matrix(float value) {
        for (float &v : mData) {
            v = value;
        }
    }

    /// Constructs matrix from buffer.
    template <typename T>
    explicit Matrix(const T *buffer) {
        for (size_t i = 0; i < mData.size(); ++i) {
            mData[i] = static_cast<float>(buffer[i]);
        }
    }

    /// Constructs matrix from brace initializer.
    template <typename T>
    Matrix(const std::initializer_list<std::initializer_list<T>> &initializer) {
        if (static_cast<int>(initializer.size()) != M) {
            throw std::invalid_argument("Mismatch between matrix number of rows");
        }

        const std::initializer_list<T> *rows = initializer.begin();
        for (int i = 0; i < M; ++i) {
            if (static_cast<int>(rows[i].size()) != N) {
                throw std::invalid_argument("Mismatch between matrix number of columns");
            }
            const T *cols = rows[i].begin();
            for (int j = 0; j < N; ++j) {
                (*this)(i, j) = static_cast<float>(cols[j]);
            }
        }
    }

    /// Returns value at specified position.
    float operator()(int row, int col) const noexcept { return mData[row * N + col]; }

    /// Returns value at specified position.
    float &operator()(int row, int col) noexcept { return mData[row * N + col]; }

    /// Returns matrix number of rows.
    constexpr int numRows() const noexcept { return M; }

    /// Returns matrix number of columns.
    constexpr int numCols() const noexcept { return N; }

    /// Returns raw pointer to matrix data.
    float *data() noexcept { return mData.data(); }

    /// Returns raw pointer to matrix data.
    const float *data() const noexcept { return mData.data(); }

    /// Computes the minimum matrix value.
    float minimum() const { return *std::min_element(mData.begin(), mData.end()); }

    /// Computes the maximum matrix value.
    float maximum() const { return *std::max_element(mData.begin(), mData.end()); }

    /// Computes the inverse of the matrix.
    Matrix<M, N> inverse() const {
        static_assert(M == 3 && N == 3, "Inverse is only implemented for 3x3 matrix.");

        const Matrix<M, N> &m = *this;

        const float det = m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) -
                          m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
                          m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));

        Matrix<M, N> minv(0.0f); // inverse of matrix m

        if (std::abs(det) < std::numeric_limits<float>::epsilon()) {
            // the matrix is not invertible, just return a null matrix
            return minv;
        }

        const float invdet = 1.0f / det;

        minv(0, 0) = (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) * invdet;
        minv(0, 1) = (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * invdet;
        minv(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invdet;
        minv(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * invdet;
        minv(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invdet;
        minv(1, 2) = (m(1, 0) * m(0, 2) - m(0, 0) * m(1, 2)) * invdet;
        minv(2, 0) = (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * invdet;
        minv(2, 1) = (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) * invdet;
        minv(2, 2) = (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * invdet;

        return minv;
    }

private:
    std::array<float, static_cast<int64_t>(M) * N> mData = {};
};

using Matrix3 = Matrix<3, 3>;

/// @}

template <int M, int N>
const Matrix<M, N> Matrix<M, N>::IDENTITY = []() {
    Matrix<M, N> id(0.0f);
    for (int i = 0; i < std::min(M, N); ++i) {
        id(i, i) = 1.0f;
    }

    return id;
}();

/// Multiplication with other matrix.
/// @relates Matrix
template <int M, int N, int O>
Matrix<M, O> operator*(const Matrix<M, N> &lhs, const Matrix<N, O> &rhs) {
    Matrix<M, O> result(0);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < O; ++j) {
            for (int k = 0; k < N; ++k) {
                result(i, j) += lhs(i, k) * rhs(k, j);
            }
        }
    }
    return result;
}

/// Multiplication with pixel.
/// @relates Matrix
template <typename T, int M, int N>
Pixel<float, M> operator*(const Matrix<M, N> &lhs, const Pixel<T, N> &rhs) {
    Pixel<float, M> pixel(0.0f);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            pixel[i] += lhs(i, j) * rhs[j];
        }
    }
    return pixel;
}

/// Multiplication with scalar.
/// @relates Matrix
template <int M, int N>
Matrix<M, N> operator*(float lhs, const Matrix<M, N> &rhs) {
    Matrix<M, N> result;
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            result(i, j) = lhs * rhs(i, j);
        }
    }

    return result;
}

} // namespace cxximg
