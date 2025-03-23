project(cxximg-util)

message(STATUS "Resolving GIT revision")

set(GIT_REVISION "unknown")
set(GIT_DESCRIBE_LONG "unknown")

find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_REVISION
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --long --dirty --all
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_DESCRIBE_LONG
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "GIT hash: ${GIT_REVISION}")
else()
    message(STATUS "GIT not found")
endif()

configure_file(revision.h.in ${CMAKE_CURRENT_BINARY_DIR}/revision.h @ONLY)

set(TARGET cxximg-util)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)

# Sources

set(SRCS ${SRC_DIR}/Version.cpp)

# Include and target definitions

add_library(${TARGET} STATIC ${SRCS})
if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(cxximg::util ALIAS ${TARGET})
endif()

target_include_directories(
    ${TARGET}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PUBLIC_HDR_DIR}> $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()
