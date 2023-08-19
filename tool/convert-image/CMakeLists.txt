project(convert-image)

set(TARGET convert-image)
set(SRC_DIR src)

# Sources

set(SRCS ${SRC_DIR}/main.cpp)

# Target definitions

add_executable(${TARGET} ${SRCS})
target_link_libraries(${TARGET} PRIVATE cxxopts::cxxopts loguru::loguru io parser util)

# Installation

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
endif()
