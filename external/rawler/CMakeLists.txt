project(rawler_ffi)

# Build Rust library using Corrosion
corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)

target_include_directories(
    rawler_ffi INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
)

if(NOT PROJECT_IS_TOP_LEVEL)
    add_library(rawler::rawler_ffi ALIAS rawler_ffi)
endif()

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS rawler_ffi EXPORT RawlerTargets)
    install(
        EXPORT RawlerTargets
        NAMESPACE rawler::
        DESTINATION cmake
    )
endif()
