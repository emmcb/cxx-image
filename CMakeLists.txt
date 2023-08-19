cmake_minimum_required(VERSION 3.21)
project(cxximg)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Limit the maximum number of parallel compilation jobs
set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=12)
set(CMAKE_JOB_POOL_COMPILE compile_job_pool)

option(CXXIMG_BUILD_TOOLS "Build the project tools" ${PROJECT_IS_TOP_LEVEL})
option(CXXIMG_ENABLE_INSTALL "Generate the install target" ${PROJECT_IS_TOP_LEVEL})

option(CXXIMG_WITH_DNG "Enable DNG format IO" ON)
option(CXXIMG_WITH_JPEG "Enable JPEG format IO" ON)
option(CXXIMG_WITH_PNG "Enable PNG format IO" ON)
option(CXXIMG_WITH_TIFF "Enable TIFF format IO" ON)

# Prevent CTest from adding default targets like Experimental / Nightly / etc.
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

include(CTest)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
endif()

add_subdirectory(third-party)
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
