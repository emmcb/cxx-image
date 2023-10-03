project(json_dto)

set(TARGET json_dto)

# Include and target definitions

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${TARGET} INTERFACE rapidjson)
