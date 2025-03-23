project(cxximg-math)

set(TARGET cxximg-math)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)
set(TEST_DIR test)

# Include and target definitions

add_library(${TARGET} INTERFACE)
if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(cxximg::math ALIAS ${TARGET})
endif()

target_include_directories(
    ${TARGET} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PUBLIC_HDR_DIR}> $<INSTALL_INTERFACE:include>
)
target_link_libraries(${TARGET} INTERFACE cxximg-util)

# Compiler flags

if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(
        ${TARGET} INTERFACE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-ffast-math> $<$<CXX_COMPILER_ID:MSVC>:/fp:fast>
    )
endif()

if(HAVE_FLOAT16)
    target_compile_definitions(${TARGET} INTERFACE CXXIMG_HAVE_FLOAT16)
endif()

target_compile_definitions(${TARGET} INTERFACE $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()

# Test

if(HAVE_GTEST AND BUILD_TESTING)
    add_executable(${TARGET}-test ${TEST_DIR}/HistogramTest.cpp)
    target_link_libraries(${TARGET}-test PRIVATE GTest::gtest_main cxximg-math)
    target_compile_options(${TARGET}-test PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-literal-conversion>)

    add_test(NAME ${TARGET}-test COMMAND ${TARGET}-test)
endif()
