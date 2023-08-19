project(parser)

set(TARGET parser)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)
set(PRIVATE_HDR_DIR src)
set(TEST_DIR test)

# Sources

set(SRCS ${SRC_DIR}/MetadataParser.cpp)

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
    PUBLIC model
    PRIVATE $<BUILD_INTERFACE:json_dto> loguru::loguru io math
)

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()

# Test

if(BUILD_TESTING)
    add_executable(${TARGET}-test ${TEST_DIR}/MetadataParserTest.cpp)
    target_link_libraries(${TARGET}-test PRIVATE GTest::gtest_main parser)

    add_test(NAME ${TARGET}-test COMMAND ${TARGET}-test)
endif()
