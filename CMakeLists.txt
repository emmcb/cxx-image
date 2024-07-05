cmake_minimum_required(VERSION 3.21)
project(cxximg)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Limit the maximum number of parallel compilation jobs
    set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=12)
    set(CMAKE_JOB_POOL_COMPILE compile_job_pool)

    # Build in release mode unless other specified
    set(DEFAULT_BUILD_TYPE "Release")
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
        set(CMAKE_BUILD_TYPE
            "${DEFAULT_BUILD_TYPE}"
            CACHE STRING "Choose the type of build." FORCE
        )
        # Set the possible values of build type for cmake-gui
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    endif()
endif()

option(CXXIMG_BUILD_TESTS "Build the project tests" ${PROJECT_IS_TOP_LEVEL})
option(CXXIMG_BUILD_TOOLS "Build the project tools" ${PROJECT_IS_TOP_LEVEL})
option(CXXIMG_ENABLE_INSTALL "Generate the install target" ${PROJECT_IS_TOP_LEVEL})

option(CXXIMG_WITH_HALIDE "Enable Halide language support" OFF)
option(CXXIMG_WITH_FLOAT16 "Enable float16 image support" OFF)

option(CXXIMG_WITH_DNG "Enable DNG format IO" ON)
option(CXXIMG_WITH_JPEG "Enable JPEG format IO" ON)
option(CXXIMG_WITH_PNG "Enable PNG format IO" ON)
option(CXXIMG_WITH_TIFF "Enable TIFF format IO" ON)

set(HAVE_GTEST 0)
if(CXXIMG_BUILD_TESTS)
    if(PROJECT_IS_TOP_LEVEL)
        # Prevent CTest from adding default targets like Experimental / Nightly / etc.
        set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

        include(CTest)
    endif()

    find_package(GTest)
    if(GTest_FOUND)
        set(HAVE_GTEST 1)
    else()
        message(WARNING "Disabling test build because GTest is not found")
    endif()
endif()

set(HAVE_HALIDE 0)
if (CXXIMG_WITH_HALIDE)
    find_package(HalideHelpers)
    if(HalideHelpers_FOUND)
        set(HAVE_HALIDE 1)
    else()
        message(WARNING "Disabling Halide language support because Halide library is not found")
    endif()
endif()

set(HAVE_FLOAT16 0)
if (CXXIMG_WITH_FLOAT16)
    # TODO check for compiler support
    set(HAVE_FLOAT16 1)
endif()

set(HAVE_DNG 0)
if(CXXIMG_WITH_DNG)
    set(HAVE_DNG 1)
endif()

set(HAVE_JPEG 0)
if(CXXIMG_WITH_JPEG)
    find_package(JPEG)
    if(JPEG_FOUND)
        set(HAVE_JPEG 1)
    else()
        message(WARNING "Disabling JPEG support because libjpeg is not found")
    endif()
endif()

set(HAVE_PNG 0)
if(CXXIMG_WITH_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(HAVE_PNG 1)
    else()
        message(WARNING "Disabling PNG support because libpng is not found")
    endif()
endif()

set(HAVE_TIFF 0)
if(CXXIMG_WITH_TIFF)
    find_package(TIFF COMPONENTS CXX)
    if(TIFF_FOUND)
        set(HAVE_TIFF 1)
    else()
        message(WARNING "Disabling TIFF support because libtiff is not found")
    endif()
endif()

set(HAVE_EXIF 0)
if(HAVE_JPEG)
    find_package(EXIF)
    if(EXIF_FOUND)
        set(HAVE_EXIF 1)
    else()
        message(WARNING "Disabling EXIF support in JPEG because libexif is not found")
    endif()
endif()

add_subdirectory(external)
add_subdirectory(lib)

if(CXXIMG_BUILD_TOOLS)
    add_subdirectory(tool)
endif()

if(CXXIMG_ENABLE_INSTALL)
    install(
        EXPORT CXXImageTargets
        NAMESPACE cxximg::
        DESTINATION cmake
    )

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        cmake/CXXImageConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CXXImageConfig.cmake INSTALL_DESTINATION cmake
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CXXImageConfig.cmake DESTINATION cmake)
endif()
