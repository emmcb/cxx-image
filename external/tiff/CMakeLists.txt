project(tiff-rational)

set(TARGET tiff-rational)

# Include and target definitions

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
