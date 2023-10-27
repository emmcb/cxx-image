project(io)

set(TARGET io)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)
set(PRIVATE_HDR_DIR src)

# Sources

set(SRCS ${SRC_DIR}/ImageIO.cpp ${SRC_DIR}/BmpIO.cpp ${SRC_DIR}/CfaIO.cpp ${SRC_DIR}/MipiRawIO.cpp
         ${SRC_DIR}/PlainIO.cpp
)

# Include and target definitions

add_library(${TARGET} STATIC ${SRCS})
if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(cxximg::${TARGET} ALIAS ${TARGET})
endif()

target_include_directories(
    ${TARGET}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PUBLIC_HDR_DIR}> $<INSTALL_INTERFACE:include>
    PRIVATE ${PRIVATE_HDR_DIR}
)
target_link_libraries(
    ${TARGET}
    PUBLIC image model
    PRIVATE loguru::loguru math util
)

# Compiler flags

if(HAVE_DNG)
    target_sources(${TARGET} PRIVATE ${SRC_DIR}/DngIO.cpp)
    target_link_libraries(${TARGET} PRIVATE DNG::dng)
    target_compile_definitions(${TARGET} PRIVATE HAVE_DNG)
endif()

if(HAVE_JPEG)
    target_sources(${TARGET} PRIVATE ${SRC_DIR}/JpegIO.cpp)
    target_link_libraries(${TARGET} PRIVATE TurboJPEG::TurboJPEG)
    target_compile_definitions(${TARGET} PRIVATE HAVE_JPEG)

    if(HAVE_EXIF)
        target_link_libraries(${TARGET} PRIVATE exif-jpeg)
    endif()
endif()

if(HAVE_PNG)
    target_sources(${TARGET} PRIVATE ${SRC_DIR}/PngIO.cpp)
    target_link_libraries(${TARGET} PRIVATE PNG::PNG)
    target_compile_definitions(${TARGET} PRIVATE HAVE_PNG)
endif()

if(HAVE_TIFF)
    target_sources(${TARGET} PRIVATE ${SRC_DIR}/TiffIO.cpp)
    target_link_libraries(${TARGET} PRIVATE TIFF::TIFF $<BUILD_INTERFACE:tiff-rational>)
    target_compile_definitions(${TARGET} PRIVATE HAVE_TIFF)
endif()

if(HAVE_EXIF)
    target_link_libraries(${TARGET} PRIVATE EXIF::EXIF)
    target_compile_definitions(${TARGET} PRIVATE HAVE_EXIF)
endif()

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()
