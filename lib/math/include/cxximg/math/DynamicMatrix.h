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

#include <stdexcept>
#include <utility>
#include <vector>

namespace cxximg {

/// Matrix with dynamic NxM size.
/// @ingroup math
class DynamicMatrix final {
public:
    /// Constructs empty matrix.
    DynamicMatrix() = default;

    /// Constructs uninitialized matrix with specified dimensions.
    DynamicMatrix(int numRows, int numCols)
        : mData(static_cast<int64_t>(numRows) * numCols), mNumRows(numRows), mNumCols(numCols) {}

    /// Constructs value-initialized matrix with specified dimensions.
    DynamicMatrix(int numRows, int numCols, float value) : DynamicMatrix(numRows, numCols) {
        for (float &v : mData) {
            v = value;
        }
    }

    /// Constructs matrix from buffer.
    template <typename T>
    DynamicMatrix(int numRows, int numCols, const T *buffer) : DynamicMatrix(numRows, numCols) {
        for (size_t i = 0; i < mData.size(); ++i) {
            mData[i] = static_cast<float>(buffer[i]);
        }
    }

    /// Constructs matrix from brace initializer.
    template <typename T>
    DynamicMatrix(const std::initializer_list<std::initializer_list<T>> &initializer) {
        const int numRows = initializer.size();
        if (numRows > 0) {
            const std::initializer_list<T> *rows = initializer.begin();
            const int numCols = rows[0].size();
            if (numCols > 0) {
                mData.resize(static_cast<int64_t>(numRows) * numCols);
                mNumRows = numRows;
                mNumCols = numCols;
                for (int i = 0; i < numRows; ++i) {
                    if (static_cast<int>(rows[i].size()) != numCols) {
                        throw std::invalid_argument("Matrix columns must have the same length");
                    }
                    const T *cols = rows[i].begin();
                    for (int j = 0; j < numCols; ++j) {
                        (*this)(i, j) = static_cast<float>(cols[j]);
                    }
                }
            }
        }
    }

    /// Returns value at specified position.
    float operator()(int row, int col) const noexcept { return mData[row * mNumCols + col]; }

    /// Returns value at specified position.
    float &operator()(int row, int col) noexcept { return mData[row * mNumCols + col]; }

    /// Returns matrix number of rows.
    int numRows() const noexcept { return mNumRows; }

    /// Returns matrix number of columns.
    int numCols() const noexcept { return mNumCols; }

    /// Returns whether the matrix is empty or not.
    bool empty() const noexcept { return mNumRows == 0 || mNumCols == 0; }

    /// Returns raw pointer to matrix data.
    float *data() noexcept { return mData.data(); }

    /// Returns raw pointer to matrix data.
    const float *data() const noexcept { return mData.data(); }

private:
    std::vector<float> mData;
    int mNumRows = 0;
    int mNumCols = 0;
};

} // namespace cxximg
