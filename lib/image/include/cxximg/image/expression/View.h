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

#include <type_traits>

namespace cxximg {

template <typename T>
class Image;

template <typename T>
class ImageView;

namespace expr {

namespace detail {

template <typename Expr>
struct View {
    using TYPE = Expr;
};

template <typename T>
struct View<Image<T> &> {
    using TYPE = ImageView<T> &;
};

template <typename T>
struct View<const Image<T> &> {
    using TYPE = const ImageView<T> &;
};

} // namespace detail

template <typename T>
using view_t = typename detail::View<T>::TYPE; // NOLINT(readability-identifier-naming)

} // namespace expr

} // namespace cxximg
