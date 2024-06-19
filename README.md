# CXX Image Library

CXX Image is a flexible and lightweight image processing library written in modern C++17.

Unlike most of its peers it does not contain any builtin image processing algorithm, because it is only meant to provide the basic building blocks that are necessary to write these algorithms.

It is primarily made of:

- An independant [Image class](lib/image) that can wrap a memory buffer of any arbitrary layout, and provides on top a set of mathematical operations using template expressions.
- An [IO library](lib/io) that supports the most common image formats, including RAW, with support of 16 bits and floating points images when available. Reading and writing additional metadata like EXIF is also supported.
- A mathematical library that includes for example Bezier curves or a generic multi-dimensional histogram class.

## CMake

This library can be added to an existing CMake project using FetchContent:

```cmake
include(FetchContent)

FetchContent_Declare(
    cxx-image
    GIT_REPOSITORY "https://github.com/emmcb/cxx-image.git"
    GIT_TAG "master"
)
FetchContent_MakeAvailable(cxx-image)
```
