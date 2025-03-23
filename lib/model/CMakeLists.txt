project(cxximg-model)

set(TARGET cxximg-model)
set(SRC_DIR src)
set(PUBLIC_HDR_DIR include)

# Include and target definitions

add_library(${TARGET} INTERFACE)
if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(cxximg::model ALIAS ${TARGET})
endif()

target_include_directories(
    ${TARGET} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PUBLIC_HDR_DIR}> $<INSTALL_INTERFACE:include>
)
target_link_libraries(${TARGET} INTERFACE cxximg-math)

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} EXPORT CXXImageTargets)
    install(DIRECTORY ${PUBLIC_HDR_DIR}/${TARGET} DESTINATION include)
endif()
