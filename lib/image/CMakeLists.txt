project(cxximg-image)

set(TARGET cxximg-image)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)
set(PRIVATE_HDR_DIR src)
set(TEST_DIR test)

# Sources

set(SRCS ${SRC_DIR}/Image.cpp ${SRC_DIR}/ImageView.cpp ${SRC_DIR}/ChannelView.cpp ${SRC_DIR}/ImageAllocator.cpp)

# Include and target definitions

add_library(${TARGET} STATIC ${SRCS})
if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(cxximg::image ALIAS ${TARGET})
endif()

target_include_directories(
    ${TARGET}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PUBLIC_HDR_DIR}> $<INSTALL_INTERFACE:include>
    PRIVATE ${PRIVATE_HDR_DIR}
)
target_link_libraries(${TARGET} PUBLIC cxximg-math cxximg-model cxximg-util)

# Compiler flags

target_compile_definitions(
    ${TARGET}
    PUBLIC CXXIMG_BASE_ALIGNMENT=${CXXIMG_BASE_ALIGNMENT}
           CXXIMG_DEFAULT_WIDTH_ALIGNMENT=${CXXIMG_DEFAULT_WIDTH_ALIGNMENT}
           CXXIMG_DEFAULT_HEIGHT_ALIGNMENT=${CXXIMG_DEFAULT_HEIGHT_ALIGNMENT}
           CXXIMG_DEFAULT_DEPTH_ALIGNMENT=${CXXIMG_DEFAULT_DEPTH_ALIGNMENT}
           CXXIMG_DEFAULT_SIZE_ALIGNMENT=${CXXIMG_DEFAULT_SIZE_ALIGNMENT}
)

if(HAVE_HALIDE)
    target_link_libraries(${TARGET} PUBLIC Halide::Runtime)
    target_compile_definitions(${TARGET} PUBLIC CXXIMG_HAVE_HALIDE)
endif()

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()

# Test

if(HAVE_GTEST AND BUILD_TESTING)
    add_executable(
        ${TARGET}-test
        ${TEST_DIR}/AllocatorTest.cpp
        ${TEST_DIR}/BorderTest.cpp
        ${TEST_DIR}/EvaluateTest.cpp
        ${TEST_DIR}/ImageTest.cpp
        ${TEST_DIR}/LayoutTest.cpp
        ${TEST_DIR}/ChannelViewTest.cpp
        ${TEST_DIR}/PixelViewTest.cpp
        ${TEST_DIR}/ResizeExpressionTest.cpp
    )
    target_link_libraries(${TARGET}-test PRIVATE GTest::gtest_main cxximg-image)

    add_test(NAME ${TARGET}-test COMMAND ${TARGET}-test)
endif()

# Vectorization checks use compiler diagnostics and do not depend on GTest or execute generated code.
if(CXXIMG_BUILD_TESTS
   AND BUILD_TESTING
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
)
    set(VECTORIZATION_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/VectorizationTestConfig-$<CONFIG>.cmake")
    file(
        GENERATE
        OUTPUT "${VECTORIZATION_CONFIG}"
        CONTENT
            "set(COMPILER [==[${CMAKE_CXX_COMPILER}]==])
set(COMPILER_ID [==[${CMAKE_CXX_COMPILER_ID}]==])
set(COMPILER_VERSION [==[${CMAKE_CXX_COMPILER_VERSION}]==])
set(COMPILER_FLAGS [==[${CMAKE_CXX_FLAGS}]==])
set(COMPILE_OPTIONS [==[$<TARGET_PROPERTY:${TARGET},COMPILE_OPTIONS>]==])
set(INCLUDE_DIRECTORIES [==[$<TARGET_PROPERTY:${TARGET},INCLUDE_DIRECTORIES>]==])
set(COMPILE_DEFINITIONS [==[$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>]==])
set(SOURCE_FILE [==[${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/VectorizationTest.cpp]==])
set(OUTPUT_DIR [==[${CMAKE_CURRENT_BINARY_DIR}/vectorization/$<CONFIG>]==])
"
            TARGET
            ${TARGET}
    )

    set(VECTORIZATION_COMMAND ${CMAKE_COMMAND} "-DCONFIG_FILE=${VECTORIZATION_CONFIG}" -P
                              "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/VectorizationTest.cmake"
    )
    add_custom_target(
        ${TARGET}-vectorization-test
        COMMAND ${VECTORIZATION_COMMAND}
        VERBATIM
    )
    add_test(NAME ${TARGET}-vectorization-test COMMAND ${VECTORIZATION_COMMAND})
    set_tests_properties(${TARGET}-vectorization-test PROPERTIES LABELS "vectorization")
endif()
