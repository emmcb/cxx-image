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

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <vector>

namespace cxximg {

namespace detail {

/// Wrapper around histogram class, that can be iterated while providing an accessor to the current bin index in
/// addition to the bin value.
template <class Histogram>
class IndexedHistogram final {
public:
    class Iterator;

    /// Constructor.
    explicit IndexedHistogram(Histogram *histogram) : mHistogram(histogram) {}

    Iterator begin() const noexcept { return Iterator(mHistogram); }
    Iterator end() const noexcept { return Iterator(mHistogram, true); }

private:
    Histogram *mHistogram;
};

template <class Histogram>
class IndexedHistogram<Histogram>::Iterator final {
    static constexpr int DIM = Histogram::DIM;
    using MultiIndex = typename Histogram::MultiIndex;

public:
    class Accessor;

    /// Prefix increment.
    Iterator &operator++() {
        for (int i = 0; i < DIM; ++i) {
            if (mIndex[i] >= mLast[i] && i < DIM - 1) {
                mIndex[i] = 0;
                continue;
            }

            ++mIndex[i];
            break;
        }
        return *this;
    }

    /// Equality test.
    bool operator==(const Iterator &iterator) const noexcept { return mIndex == iterator.mIndex; }
    bool operator!=(const Iterator &iterator) const noexcept { return !operator==(iterator); }

    /// Gets the value.
    Accessor operator*() const noexcept { return Accessor(this); }

private:
    friend class IndexedHistogram;

    /// Constructor.
    explicit Iterator(Histogram *histogram, bool end = false) : mHistogram(histogram) {
        std::fill(mIndex.begin(), mIndex.end(), 0);
        std::apply([&](const auto &...axis) { mLast = MultiIndex{(axis.size() - 1)...}; }, mHistogram->axes());

        if (end) {
            mIndex[DIM - 1] = mLast[DIM - 1] + 1;
        }
    }

    Histogram *mHistogram;
    MultiIndex mIndex;
    MultiIndex mLast;
};

template <class Histogram>
class IndexedHistogram<Histogram>::Iterator::Accessor final {
public:
    /// Returns the current bin value.
    decltype(auto) operator*() const { return mIterator->mHistogram->at(mIterator->mIndex); }

    /// Returns the current index for the N-th axis.
    template <std::size_t N = 0>
    int index() const {
        return mIterator->mIndex[N];
    }

    /// Returns the current bin coordinate for the N-th axis.
    template <std::size_t N = 0>
    decltype(auto) interval() const {
        return mIterator->mHistogram->template axis<N>().interval(mIterator->mIndex[N]);
    }

private:
    friend class Iterator;

    /// Constructor.
    explicit Accessor(const Iterator *iterator) : mIterator(iterator) {}

    const Iterator *mIterator;
};

/// Generic multi-dimensional histogram class.
template <typename T, typename U, template <typename> class... Axis>
class GenericHistogram {
    static constexpr int UNDERFLOW_BIN = 1;
    static constexpr int OVERFLOW_BIN = 1;

public:
    static constexpr int DIM = sizeof...(Axis);
    static_assert(DIM > 0);

    using MultiCoord = std::array<T, DIM>;
    using MultiIndex = std::array<int, DIM>;

    /// Constructs an histogram given the provided axes.
    explicit GenericHistogram(const Axis<T> &...axes) : mAxes{axes...}, mData(size(), U(0)) {}

    /// Returns the histogram value at the specified 1d index.
    U &operator[](int index) { return mData[index + UNDERFLOW_BIN]; }

    /// Returns the histogram value at the specified 1d index.
    U operator[](int index) const { return mData[index + UNDERFLOW_BIN]; }

    /// Returns the histogram value at the specified index.
    U &operator[](const MultiIndex &index) { return at(index); }

    /// Returns the histogram value at the specified index.
    U operator[](const MultiIndex &index) const { return at(index); }

    /// Returns the histogram axes.
    const auto &axes() const { return mAxes; }

    /// Returns the N-th axis.
    template <std::size_t N = 0>
    const auto &axis() const {
        return std::get<N>(mAxes);
    }

    /// Returns the histogram value at the specified index.
    template <typename... Index>
    U &at(Index... index) {
        return at({index...});
    }

    /// Returns the histogram value at the specified index.
    template <typename... Index>
    U at(Index... index) const {
        return at({index...});
    }

    /// Returns the histogram value at the specified index.
    U &at(const MultiIndex &index) {
        const int idx = linearizeIndex(index);
        return mData[idx];
    }

    /// Returns the histogram value at the specified index.
    U at(const MultiIndex &index) const {
        const int idx = linearizeIndex(index);
        return mData[idx];
    }

    /// Returns the histogram value at the specified coordinate.
    template <typename... Coord>
    U &count(Coord... coord) {
        return count({static_cast<T>(coord)...});
    }

    /// Returns the histogram value at the specified coordinate.
    template <typename... Coord>
    U count(Coord... coord) const {
        return count({static_cast<T>(coord)...});
    }

    /// Returns the histogram value at the specified coordinate.
    U &count(const MultiCoord &coord) {
        const int idx = coordIndex(coord);
        return mData[idx];
    }

    /// Returns the histogram value at the specified coordinate.
    U count(const MultiCoord &coord) const {
        const int idx = coordIndex(coord);
        return mData[idx];
    }

    /// Returns an histogram that can be iterated while providing an accessor to
    /// the current bin index in addition to the bin value.
    auto indexed() { return IndexedHistogram<GenericHistogram<T, U, Axis...>>(this); }

    /// Returns an histogram that can be iterated while providing an accessor to
    /// the current bin index in addition to the bin value.
    auto indexed() const { return IndexedHistogram<const GenericHistogram<T, U, Axis...>>(this); }

    std::vector<U> &data() { return mData; }
    const std::vector<U> &data() const { return mData; }

    typename std::vector<U>::iterator begin() { return mData.begin(); }
    typename std::vector<U>::iterator end() { return mData.end(); }
    typename std::vector<U>::const_iterator begin() const { return mData.begin(); }
    typename std::vector<U>::const_iterator end() const { return mData.end(); }

protected:
    /// Returns the histogram total number of bins.
    template <typename S = std::make_index_sequence<DIM>>
    int size() const noexcept {
        return sizeImpl(S{});
    }

    /// Returns the index in storage of the given axis coordinates.
    int coordIndex(const MultiCoord &coord) const noexcept {
        return coordIndexImpl(coord, std::make_index_sequence<DIM>{});
    }

    /// Returns the linearized index of the given axis indices.
    int linearizeIndex(const MultiIndex &index) const noexcept {
        return linearizeIndexImpl(index, std::make_index_sequence<DIM>{});
    }

private:
    template <std::size_t... I>
    int sizeImpl(std::index_sequence<I...>) const noexcept { // NOLINT(readability-named-parameter)
        return (1 * ... * (axis<I>().size() + UNDERFLOW_BIN + OVERFLOW_BIN));
    }

    template <std::size_t... I>
    int coordIndexImpl(const MultiCoord &coord,
                       std::index_sequence<I...>) const noexcept { // NOLINT(readability-named-parameter)
        return (... + (size<std::make_index_sequence<I>>() * (axis<I>().index(coord[I]) + UNDERFLOW_BIN)));
    }

    template <std::size_t... I>
    int linearizeIndexImpl(const MultiIndex &index,
                           std::index_sequence<I...>) const noexcept { // NOLINT(readability-named-parameter)
        return (... + (size<std::make_index_sequence<I>>() * (index[I] + UNDERFLOW_BIN)));
    }

    std::tuple<Axis<T>...> mAxes;
    std::vector<U> mData;
};

} // namespace detail

/// @addtogroup math
/// @{

template <typename T, typename U, template <typename> class Axis>
class CumulativeHistogram final : public detail::GenericHistogram<T, U, Axis> {
public:
    using detail::GenericHistogram<T, U, Axis>::GenericHistogram;

    /// Returns the total number of counts.
    U totalCount() const { return this->at(this->axis().size()); }

    /// Interpolates the coordinate corresponding to the given count.
    float coord(float count) const {
        const auto &axis = this->axis();
        if (count <= this->at(0)) {
            return axis.coord(0.5f);
        }

        for (int i = 1; i < axis.size(); ++i) {
            const U &prev = this->at(i - 1);
            const U &cur = this->at(i);
            if (count > prev && count <= cur) {
                const float k = (count - prev) / (cur - prev);
                return (1.0f - k) * axis.coord(i - 0.5f) + k * axis.coord(i + 0.5f);
            }
        }

        return axis.coord(axis.size() - 0.5f);
    }
};

template <typename T, typename U, template <typename> class... Axis>
class Histogram final : public detail::GenericHistogram<T, U, Axis...> {
public:
    using detail::GenericHistogram<T, U, Axis...>::GenericHistogram;

    /// Inserts value in histogram.
    template <typename... Coord>
    void operator()(Coord... coord) {
        this->count(coord...) += U(1);
    }

    /// Computes the total number of counts.
    U totalCount() const { return std::accumulate(this->begin(), this->end(), U(0)); }

    /// Computes the distribution mean along the given axis.
    template <std::size_t N = 0>
    float mean() const {
        float sum = 0.0f;
        float totalCount = 0.0f;
        for (const auto &bin : this->indexed()) {
            const T coord = bin.template interval<N>().center();

            sum += static_cast<float>(coord) * (*bin);
            totalCount += (*bin);
        }

        return sum / totalCount;
    }

    /// Computes the distribution mean along the given axis, restrained to the
    /// [from, to] interval.
    template <std::size_t N = 0>
    float mean(T from, T to) const {
        float sum = 0.0f;
        float totalCount = 0.0f;
        for (const auto &bin : this->indexed()) {
            const T coord = bin.template interval<N>().center();
            if (coord < from || coord > to) {
                continue;
            }

            sum += static_cast<float>(coord) * (*bin);
            totalCount += (*bin);
        }

        return sum / totalCount;
    }

    /// Computes the cumulative histogram.
    auto accumulated() const {
        static_assert(sizeof...(Axis) == 1);

        CumulativeHistogram<T, U, Axis...> hist(this->template axis<0>());
        std::partial_sum(this->begin(), this->end(), hist.begin());

        return hist;
    }
};

/// Histogram functions
namespace hist {

/// Histogram axis with regularly spaced bins.
template <typename T>
class RegularAxis final {
    static constexpr T EPS = std::is_integral_v<T> ? T(1) : T(0);

public:
    class Interval;

    /// Axis constructor.
    RegularAxis(int size, T from, T to) : mSize(size), mFrom(from), mTo(to), mDelta(to - from + EPS) {}

    /// Computes index for given coordinate.
    int index(T coord) const noexcept {
        if (coord < mFrom) {
            return -1;
        }
        if (coord == mTo) {
            return mSize - 1; // upper edge of last bin is inclusive
        }
        if (coord > mTo) {
            return mSize;
        }
        const float t = (coord - mFrom) / mDelta;
        return static_cast<int>(t * mSize);
    }

    /// Computes coordinate for given floating point index.
    T coord(float index) const noexcept {
        const float t = index / mSize;
        return static_cast<T>(mFrom + mDelta * t);
    }

    /// Computes interval for given index.
    Interval interval(int index) const noexcept { return Interval(this, index); }

    /// Returns axis size.
    int size() const noexcept { return mSize; }

private:
    int mSize;
    T mFrom;
    T mTo;
    float mDelta;
};

template <typename T>
class RegularAxis<T>::Interval final {
public:
    /// Lower limit of the interval.
    T lower() const noexcept { return mAxis->coord(mIndex); }

    /// Upper limit of the interval.
    T upper() const noexcept { return mAxis->coord(mIndex + 1.0f); }

    /// Center of the interval.
    T center() const noexcept { return mAxis->coord(mIndex + 0.5f); }

    /// Interval width.
    T width() const noexcept { return upper() - lower(); }

private:
    friend class RegularAxis;

    Interval(const RegularAxis<T> *axis, int index) : mAxis(axis), mIndex(static_cast<float>(index)) {}

    const RegularAxis<T> *mAxis;
    float mIndex;
};

/// Constructs a new histogram from given axes.
template <typename U, typename T, template <typename> class... Axis>
Histogram<T, U, Axis...> makeHistogram(const Axis<T> &...axes) {
    return Histogram<T, U, Axis...>(axes...);
}

/// Constructs a new histogram from given axes, using default unsigned integer type for counts.
template <typename T, template <typename> class... Axis>
Histogram<T, unsigned, Axis...> makeHistogram(const Axis<T> &...axes) {
    return Histogram<T, unsigned, Axis...>(axes...);
}

} // namespace hist

using Histogram8u = Histogram<uint8_t, unsigned, hist::RegularAxis>;
using Histogram16u = Histogram<uint16_t, unsigned, hist::RegularAxis>;
using Histogramf = Histogram<float, unsigned, hist::RegularAxis>;

/// @}

}; // namespace cxximg
