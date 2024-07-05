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

#include "image/Image.h"

namespace cxximg {

LayoutDescriptor const LayoutDescriptor::EMPTY{};

template class Image<int8_t>;
template class Image<int16_t>;
template class Image<int32_t>;

template class Image<uint8_t>;
template class Image<uint16_t>;
template class Image<uint32_t>;

#ifdef HAVE_FLOAT16
template class Image<half_t>;
#endif
template class Image<float>;
template class Image<double>;

} // namespace cxximg
