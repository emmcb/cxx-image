Image metadata parser {#libparser}
====================

@tableofcontents

The [Parser library](@ref parser) is a C++ library which allows to read and write the image metadata from a JSON file to the cxximg::ImageMetadata structure.

# Read image metadata from JSON

Given a path to a JSON file, we can read the image metadata using cxximg::parser::readMetadata:

~~~~~~~~~~~~~~~{.cpp}
#include "parser/MetadataParser.h"

using namespace cxximg;

ImageMetadata metadata = parser::readMetadata("imageMetadata.json");
~~~~~~~~~~~~~~~

Often the image metadata will be stored in a sidecar file next to the image, that is to say a JSON file with the same name and in the same folder than the image:

~~~~~~~~~~~~~~~{.cpp}
std::optional<ImageMetadata> metadata = parser::readMetadata("image.raw", std::nullopt);

if (metadata) {
    // The image has a sidecar next to it.
}
~~~~~~~~~~~~~~~

# Write image metadata to JSON

it is also possible to serialize back the metadata to a JSON file:

~~~~~~~~~~~~~~~{.cpp}
#include "parser/MetadataParser.h"

ImageMetadata metadata;
parser::writeMetadata(metadata, "output_metadata.json");
~~~~~~~~~~~~~~~
