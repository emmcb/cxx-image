project(image)

set(TARGET image)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)
set(PRIVATE_HDR_DIR src)
set(TEST_DIR test)

# Sources

set(SRCS ${SRC_DIR}/Image.cpp ${SRC_DIR}/ImageView.cpp ${SRC_DIR}/PlaneView.cpp)

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
target_link_libraries(${TARGET} PUBLIC math model util)

# Compiler flags

target_compile_definitions(
    ${TARGET}
    PUBLIC CXXIMG_BASE_ALIGNMENT=${CXXIMG_BASE_ALIGNMENT}
           CXXIMG_DEFAULT_WIDTH_ALIGNMENT=${CXXIMG_DEFAULT_WIDTH_ALIGNMENT}
           CXXIMG_DEFAULT_HEIGHT_ALIGNMENT=${CXXIMG_DEFAULT_HEIGHT_ALIGNMENT}
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
        ${TARGET}-test ${TEST_DIR}/BorderTest.cpp ${TEST_DIR}/ImageTest.cpp ${TEST_DIR}/LayoutTest.cpp
                       ${TEST_DIR}/PlaneViewTest.cpp ${TEST_DIR}/ResizeExpressionTest.cpp
    )
    target_link_libraries(${TARGET}-test PRIVATE GTest::gtest_main image)

    add_test(NAME ${TARGET}-test COMMAND ${TARGET}-test)
endif()
