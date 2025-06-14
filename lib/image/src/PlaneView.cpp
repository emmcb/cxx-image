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

#include "cxximg/image/view/PlaneView.h"

namespace cxximg {

template class PlaneView<int8_t>;
template class PlaneView<int16_t>;
template class PlaneView<int32_t>;

template class PlaneView<uint8_t>;
template class PlaneView<uint16_t>;
template class PlaneView<uint32_t>;

#ifdef CXXIMG_HAVE_FLOAT16
template class PlaneView<half>;
#endif
template class PlaneView<float>;
template class PlaneView<double>;

} // namespace cxximg
