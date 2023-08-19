project(rapidjson)

set(TARGET rapidjson)

# Include and target definitions

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Compiler flags

target_compile_definitions(${TARGET} INTERFACE RAPIDJSON_HAS_STDSTRING)
