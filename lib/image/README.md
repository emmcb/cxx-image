Image storage and manipulation {#libimage}
======================================

@tableofcontents

The [Image library](@ref image) is the cornerstone of the image processing algorithms. It provides an automatic memory management and allows to manipulate the image pixels using high-level expressions, by abstracting the internal pixel buffer.

# Image storage

The image library supports two mode of operation:

* Either we can allocate a new image in memory by constructing a cxximg::Image instance. The instance will own the allocated memory, which will be freed in the destructor.
* Either we can wrap an existing buffer by constructing a cxximg::ImageView instance. In this case we do not own the memory, that has to be managed and freed externally. This mode is useful to wrap and modify in place an image buffer that is provided by a third-party application.

Whatever the operation mode, the user must give to the image library the needed informations about the image (width, height, pixel type, ...) and how are organised the pixels in the buffer (planar, interleaved, ...).<br>This done using cxximg::LayoutDescriptor and cxximg::ImageDescriptor structures, that are discussed more in depth below.

## Buffer descriptor

cxximg::LayoutDescriptor structure can be used for simple description of the image layout. It must be constructed using the provided builder cxximg::LayoutDescriptor::Builder. For example:

~~~~~~~~~~~~~~~{.cpp}
#include "image/Image.h"

using namespace cxximg;

// Describes a 4000x3000 interleaved RGB image.
// The pixel will be internally organised as RGBRGBRGB... (= interleaved).
LayoutDescriptor descriptor = LayoutDescriptor::Builder(4000, 3000)
                                      .imageLayout(ImageLayout::INTERLEAVED)
                                      .pixelType(PixelType::RGB)
                                      .build();

// Describes a 4000x3000 YUV 420 image.
// The pixel will be internally organised as YYYYYY... then UUU... then VVV... (= planar), with subsampled U and V (YUV 420).
LayoutDescriptor descriptor = LayoutDescriptor::Builder(4000, 3000)
                                      .imageLayout(ImageLayout::YUV_420)
                                      .build();

// Describes a 12 bits 4000x3000 RAW image with bayer phase RGGB.
LayoutDescriptor descriptor = LayoutDescriptor::Builder(4000, 3000)
                                      .pixelType(PixelType::BAYER_RGGB)
                                      .pixelPrecision(12)
                                      .build();
~~~~~~~~~~~~~~~

## Allocating an image

Given a descriptor, we can allocate an image by constructing a new cxximg::Image instance. Some aliases are predefined for common types, like ```Image8u``` (8 bits unsigned values), ```Image16u``` (16 bits unsigned values), or ```Imagef``` (floating point values).

~~~~~~~~~~~~~~~{.cpp}
{
    // Allocates a 16 bits unsigned 4000x3000 RGB image, without initializing the pixels.
    Image16u rgb(LayoutDescriptor::Builder(4000, 3000).pixelType(PixelType::RGB).build());

} // Image data is freed when the Image instance is destructed at the end of the scope.
~~~~~~~~~~~~~~~

cxximg::Image instances can be moved using [std::move](https://en.cppreference.com/w/cpp/utility/move), however implicit copies are forbidden. Copies must be done explicitely by calling cxximg::image::clone.

~~~~~~~~~~~~~~~{.cpp}
// Allocates a new image
Image16u img1(...)

Image16u img2 = img1;                // KO: does not compile, implicit copies are forbidden.
Image16u img2 = image::clone(img1);  // OK: explicit copy.
Image16u img2 = std::move(img1);     // OK: data is moved from img1 to img2 without copy. img1 cannot be used anymore after this line.
~~~~~~~~~~~~~~~

## Wrapping an existing buffer

An existing buffer can be wrapped by constructing directly a new cxximg::ImageView instance.

~~~~~~~~~~~~~~~{.cpp}
// Buffer created externally, for example by a third-party library.
uint16_t* buffer = new uint16_t[4000 * 3000 * 3];

{
    // Constructs a 16 bits unsigned 4000x3000 RGB image view referencing an existing buffer.
    ImageView16u rgb(LayoutDescriptor::Builder(4000, 3000).pixelType(PixelType::RGB).build(), buffer);

} // Buffer is NOT freed when the ImageView instance is destructed at the end of the scope.

// It is the responsability of the user to free the buffer when it is not used anymore.
delete[] buffer;
~~~~~~~~~~~~~~~

The cxximg::ImageView class is more generic than the cxximg::Image class, thus an cxximg::Image instance can be downcasted to an cxximg::ImageView instance, but the reverse is not true.

Consequently, image processing functions should generally work on cxximg::ImageView inputs so that they can stay more generic:

~~~~~~~~~~~~~~~{.cpp}
void processImage(ImageView16u& img) {
    // Do processing on img
}

// Allocates a new RGB image.
Image16u rgb(LayoutDescriptor::Builder(4000, 3000).pixelType(PixelType::RGB).build());

// Do processing
processImage(rgb); // OK: Image16u is implicitely downcasted to ImageView16u.
~~~~~~~~~~~~~~~

# Image manipulation

The image library provides high-level ways of manipulating the pixels, without having to rely on manual for-loops.

## Expressions

An expression is a set of mathematical operations that will be applied to the image. The definition of the expression is decoupled from the expression evaluation, that allows for efficient operations both in terms of memory and running time.

### Base operators

Base mathematical operators like +,-,*,/ are usable with expressions. See cxximg::expr::BaseExpression documentation for the complete list of operators.

~~~~~~~~~~~~~~~{.cpp}
// Allocates a new grayscale image filled with the value 1.
Image16u img(LayoutDescriptor::Builder(4000, 3000).pixelType(PixelType::GRAYSCALE).build(), 1);

// Do some mathematical operations.
img = 2 * img + 1;

// Now the image is filled with the value 2 * 1 + 1 = 3
~~~~~~~~~~~~~~~

### Predefined expressions

Many useful expressions are already predefined by the image library. See the @ref cxximg::expr namespace documentation for the complete list of expressions.

~~~~~~~~~~~~~~~{.cpp}
// Takes the square root of the image values, round the result, and add one.
img = expr::lround(expr::sqrt(img)) + 1;
~~~~~~~~~~~~~~~

It is also possible to separate the definition from the evaluation of the expression:

~~~~~~~~~~~~~~~{.cpp}
// Here we define the expression, but no evaluation is done and no memory is allocated.
auto expression = expr::lround(expr::sqrt(img)) + 1;

// Do the actual expression evaluation
img = expression;
~~~~~~~~~~~~~~~

### Lambda expressions

The user can easily define its own expressions using C++ lambdas and cxximg::expr::evaluate:

~~~~~~~~~~~~~~~{.cpp}
img = [&](auto... coords) {
    uint16_t inPixel = expr::evaluate(img, coords...);
    uint16_t outPixel = ...; // do some processing

    return outPixel;
};
~~~~~~~~~~~~~~~

## Region subset

It is possible to limit the processing to an image region by subsetting a cxximg::Roi:

~~~~~~~~~~~~~~~{.cpp}
ImageView16u imgRoi = img[{0, 0, 100, 100}]; // 100x100 rectangle located at (0,0)

imgRoi += 1; // Only the top left rectangle of img is processed.
~~~~~~~~~~~~~~~

## Handling borders

Expression evaluation do not check image bounds during processing, thus the user must ensure that no out-of-bounds access will occur if necessary. The image library provides two ways to do bound checking:

### Using an expression

The idea is to wrap the image into an expression that will do the bound checking when accessing the image values. No additional memory is allocated, however image manipulation will be slower due to the extra cost of the conditions.

~~~~~~~~~~~~~~~{.cpp}
// Wraps the input image into an expression that will do the bound checking.
auto imgWithBoundCheck = expr::border<BorderMode::MIRROR>(img);

// Now we are safe to look outside bounds.
img2 = [&](int x, int y, auto... coords) {
    uint16_t leftPixel = expr::evaluate(imgWithBoundCheck, x - 1, y, coords...);
    uint16_t rightPixel = expr::evaluate(imgWithBoundCheck, x + 1, y, coords...);

    return (rightPixel + leftPixel) / 2;
};
~~~~~~~~~~~~~~~

### Using precomputed borders

Another option is to allocate a bigger buffer with an extra space for the borders, that can be precomputed before evaluating the expression. The advantage is that once the border has been precomputed, no additional runtime cost will occur. cxximg::image::makeBorders and cxximg::image::updateBorders will take care of these steps.

~~~~~~~~~~~~~~~{.cpp}
#include "cxximg/image/function/Border.h"

// Allocate a new image with 1px border.
Image16u imgWithBorder = image::makeBorders<BorderMode::MIRROR>(img, 1);

// Note that the width and height of imgWithBorder is the same than the width and height of img.
// However, we can now access to the coordinate x = -1 or y = -1 due to the extra 1px border.

// Now we are safe to look 1px outside bounds.
img = [&](int x, int y, auto... coords) {
    uint16_t leftPixel = expr::evaluate(imgWithBorder, x - 1, y, coords...);
    uint16_t rightPixel = expr::evaluate(imgWithBorder, x + 1, y, coords...);

    return (rightPixel + leftPixel) / 2;
};
~~~~~~~~~~~~~~~
