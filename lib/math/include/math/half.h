#pragma once

namespace cxximg {

#if defined(HAVE_FLOAT16)
using half_t = _Float16;
#else
using half_t = float;
#endif

} // namespace cxximg
