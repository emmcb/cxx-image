Image IO {#libio}
=================

@tableofcontents

The [Image IO library](@ref io) allows to read and write images in many file formats, while designed to be generic and to interact nicely with the [image library](@ref image).

| Image format  | Read | Write | EXIF | Pixel precision        | Pixel type           | File extension                   |
|---------------|------|-------|------|------------------------|----------------------|----------------------------------|
| BMP           | x    | x     |      | 8 bits                 | Grayscale, RGB, RGBA | .bmp                             |
| CFA           | x    | x     |      | 16 bits                | Bayer                | .cfa                             |
| DNG           | x    | x     | x    | 16 bits, float         | Bayer, RGB           | .dng                             |
| JPEG          | x    | x     | x    | 8 bits                 | Grayscale, RGB, YUV  | .jpg, .jpeg                      |
| MIPIRAW       | x    | x     |      | 10 bits, 12 bits       | Bayer                | .RAWMIPI, .RAWMIPI10, .RAWMIPI12 |
| PLAIN         | x    | x     |      | *                      | *                    | .nv12, .plain16, .y8, *          |
| PNG           | x    | x     |      | 8 bits, 16 bits        | Grayscale, RGB, RGBA | .png                             |
| TIFF          | x    | x     | x    | 8 bits, 16 bits, float | Bayer, RGB           | .tif, .tiff                      |

In addition to this, if it is built with [Rawler](https://github.com/dnglab/dnglab) library support, then it will be able to read the RAW formats from all the major camera manufacters.

# Image reading

## Creating the image reader

cxximg::io::makeReader() will take care of instancing a new cxximg::ImageReader able to read the image file passed as argument.

~~~~~~~~~~~~~~~{.cpp}
#include "io/ImageIO.h"

using namespace cxximg;

std::unique_ptr<ImageReader> imageReader = io::makeReader("/path/to/image.jpg");
~~~~~~~~~~~~~~~

Some file formats need to know in advance some informations about the image.<br>
For example, the PLAIN format is just a simple dump of a buffer into a file, thus it needs to know how to interpret the data. In this case we can use the cxximg::ImageReader::Options structure in order to provide the desired data:

~~~~~~~~~~~~~~~{.cpp}
ImageReader::Options options;
options.fileInfo.fileFormat = FileFormat::PLAIN;
options.fileInfo.width = 4000;
options.fileInfo.height = 3000;
options.fileInfo.imageLayout = ImageLayout::NV12; // Semi planar YUV

std::unique_ptr<ImageReader> imageReader = io::makeReader("/path/to/image.nv12", options);
~~~~~~~~~~~~~~~

We can also rely on the [parser library](@ref parser) in order to provide the informations through an image sidecar JSON located next to the image file:

~~~~~~~~~~~~~~~{.json}
{
    "fileInfo": {
        "fileFormat": "plain",
        "width": 4000,
        "height": 3000,
        "imageLayout": "nv12"
    }
}
~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~{.cpp}
#include "parser/MetadataParser.h"

std::optional<ImageMetadata> metadata = parser::readMetadata("/path/to/image.nv12", {});
std::unique_ptr<ImageReader> imageReader = io::makeReader("/path/to/image.nv12", ImageReader::Options(metadata));
~~~~~~~~~~~~~~~

## Reading the image

Once we have a reader, the file can be read and decoded as an cxximg::Image object using the following methods, depending on the image pixel data:
* 8 bits integer image: cxximg::ImageReader::read8u()
* 16 bits integer image: cxximg::ImageReader::read16u()
* floating point image: cxximg::ImageReader::readf()

If necessary, cxximg::ImageReader::pixelRepresentation() and cxximg::ImageReader::layoutDescriptor() can be used to get header informations like width, height or and pixel type. This can be useful to do some checks without having to read the entire image.

~~~~~~~~~~~~~~~{.cpp}
if (imageReader->pixelRepresentation() != PixelRepresentation::UINT16) {
    ABORT_S() << "Expecting 16 bits image";
}
if (imageReader->layoutDescriptor().pixelType != PixelType::RGB) {
    ABORT_S() << "Expecting RGB image";
}

Image16u rgb = imageReader->read16u(); // 16 bits read
~~~~~~~~~~~~~~~

# Image writing

## Creating the image writer

cxximg::io::makeWriter() will take care of instancing a new cxximg::ImageWriter able to write an image to the file passed as argument.

~~~~~~~~~~~~~~~{.cpp}
std::unique_ptr<ImageWriter> imageWriter = io::makeWriter("/path/to/image.jpg");
~~~~~~~~~~~~~~~

The correct image format is determined based on the file extension. However, some formats does not rely on a specific extension, for example the PLAIN format that allows to directly dump the image buffer to a file. In this case, the format can be specified through ImageWriter::Options.

~~~~~~~~~~~~~~~{.cpp}
ImageWriter::Options options;
options.fileFormat = FileFormat::PLAIN;

std::unique_ptr<ImageWriter> imageWriter = io::makeWriter("/path/to/image.bin", options);
~~~~~~~~~~~~~~~

## Writing the image

Once we have a writer, an image can encoded and written to the file using cxximg::ImageWriter::write().

~~~~~~~~~~~~~~~{.cpp}
Image16u rgb = ...

imageWriter->write(rgb);
~~~~~~~~~~~~~~~

# EXIF

Some image formats, like JPEG and TIFF, support EXIF reading and writing.

If supported, EXIF can be read by calling cxximg::ImageReader::readExif() on the image reader.

~~~~~~~~~~~~~~~{.cpp}
std::optional<ExifMetadata> exif = imageReader->readExif();
~~~~~~~~~~~~~~~

EXIF metadata can be written along with an image by specifying them in the image writer options. In this case, the EXIF wil be written when calling cxximg::ImageWriter::write().

~~~~~~~~~~~~~~~{.cpp}
ExifMetadata exif;

ImageWriter::Options options;
options.metadata = ImageMetadata{
    .exifMetadata = exif;
};

io::makeWriter("image.jpg", options)->write(image); // this will also write the EXIF
~~~~~~~~~~~~~~~

It is also possible to write only the EXIF by calling cxximg::ImageWriter::writeExif(). It is useful to change the EXIF of an existing image, without re-encoding the image.

~~~~~~~~~~~~~~~{.cpp}
ExifMetadata exif;

io::makeWriter("/path/to/an/existing/image.jpg")->writeExif(exif);
~~~~~~~~~~~~~~~
