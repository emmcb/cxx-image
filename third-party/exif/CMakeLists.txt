project(exif-jpeg)

set(TARGET exif-jpeg)

# Sources

set(SRCS libjpeg/jpeg-data.c libjpeg/jpeg-marker.c)

# Include and target definitions

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(${TARGET} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${TARGET} PUBLIC EXIF::EXIF)
