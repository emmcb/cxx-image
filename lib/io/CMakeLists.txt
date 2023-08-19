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

if(CXXIMG_WITH_DNG)
    target_sources(${TARGET} PRIVATE ${SRC_DIR}/DngIO.cpp)
    target_link_libraries(${TARGET} PRIVATE DNG::dng)
    target_compile_definitions(${TARGET} PRIVATE WITH_DNG)
endif()

if(CXXIMG_WITH_JPEG)
    find_package(TurboJPEG)
    if(TurboJPEG_FOUND)
        target_sources(${TARGET} PRIVATE ${SRC_DIR}/JpegIO.cpp)
        target_link_libraries(${TARGET} PRIVATE TurboJPEG::TurboJPEG)
        target_compile_definitions(${TARGET} PRIVATE WITH_JPEG)

        find_package(EXIF)
        if(EXIF_FOUND)
            target_link_libraries(${TARGET} PRIVATE EXIF::EXIF exif-jpeg)
            target_compile_definitions(${TARGET} PRIVATE WITH_JPEG_EXIF)
        else()
            message(WARNING "Disabling EXIF support in JPEG because libexif is not found")
        endif()
    else()
        message(WARNING "Disabling JPEG support because turbojpeg is not found")
    endif()
endif()

if(CXXIMG_WITH_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        target_sources(${TARGET} PRIVATE ${SRC_DIR}/PngIO.cpp)
        target_link_libraries(${TARGET} PRIVATE PNG::PNG)
        target_compile_definitions(${TARGET} PRIVATE WITH_PNG)
    else()
        message(WARNING "Disabling PNG support because libpng is not found")
    endif()
endif()

if(CXXIMG_WITH_TIFF)
    find_package(TIFF)
    if(TIFF_FOUND)
        target_sources(${TARGET} PRIVATE ${SRC_DIR}/TiffIO.cpp)
        target_link_libraries(${TARGET} PRIVATE TIFF::TIFF $<BUILD_INTERFACE:tiff-rational>)
        target_compile_definitions(${TARGET} PRIVATE WITH_TIFF)
    else()
        message(WARNING "Disabling TIFF support because libtiff is not found")
    endif()
endif()

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()
