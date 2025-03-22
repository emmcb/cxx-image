include(FetchContent)

# Loguru

set(LOGURU_STACKTRACES 0)
set(LOGURU_WITH_STREAMS 1)

FetchContent_Declare(
    loguru
    GIT_REPOSITORY "https://github.com/emmcb/loguru.git"
    GIT_TAG "972f4f42b402aa9507efaeef725ef877cd3e1ce5"
)
FetchContent_MakeAvailable(loguru)

get_target_property(LOGURU_INCLUDE_DIRECTORIES loguru INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(loguru PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LOGURU_INCLUDE_DIRECTORIES}")

if(CXXIMG_ENABLE_INSTALL)
    install(TARGETS loguru EXPORT LoguruTargets)
    install(
        EXPORT LoguruTargets
        NAMESPACE loguru::
        DESTINATION cmake
    )
endif()

# DNG

if(CXXIMG_WITH_DNG)
    set(DNG_THREAD_SAFE 0)
    set(DNG_WITH_JPEG 0)
    set(DNG_WITH_XMP 0)

    FetchContent_Declare(
        adobe-dng-sdk
        GIT_REPOSITORY "https://github.com/emmcb/adobe-dng-sdk.git"
        GIT_TAG "d83821f555c3cb746aaf97d19c404045307b850c"
    )
    FetchContent_MakeAvailable(adobe-dng-sdk)

    get_target_property(DNG_INCLUDE_DIRECTORIES dng INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(dng PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${DNG_INCLUDE_DIRECTORIES}")

    if(CXXIMG_ENABLE_INSTALL)
        install(TARGETS dng EXPORT DNGTargets)
        install(
            EXPORT DNGTargets
            NAMESPACE DNG::
            DESTINATION cmake
        )
    endif()
endif()

# RawSpeed

if(CXXIMG_WITH_RAWSPEED)
    set(BINARY_PACKAGE_BUILD 1)

    FetchContent_Declare(
        rawspeed
        GIT_REPOSITORY "https://github.com/darktable-org/rawspeed.git"
        GIT_TAG "2856186380b20efbd858d8f7bf0c334597b9b9cd"
        SOURCE_SUBDIR dummy
    )
    FetchContent_MakeAvailable(rawspeed)

    # Replace the upstream CMakeLists.txt with our own
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RawSpeedCMakeLists.txt ${rawspeed_SOURCE_DIR}/CMakeLists.txt COPYONLY
    )

    add_subdirectory(${rawspeed_SOURCE_DIR} ${rawspeed_BINARY_DIR} SYSTEM)

    foreach(
        TARGET
        rawspeed_adt
        rawspeed_codes
        rawspeed_common
        rawspeed_decoders
        rawspeed_decompressors
        rawspeed_interpolators
        rawspeed_io
        rawspeed_metadata
        rawspeed_parsers
        rawspeed_tiff
        rawspeed
    )
        get_target_property(RAWSPEED_INCLUDE_DIRECTORIES ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT "${RAWSPEED_INCLUDE_DIRECTORIES}" MATCHES "NOTFOUND")
            set_target_properties(
                ${TARGET} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${RAWSPEED_INCLUDE_DIRECTORIES}>"
            )
        endif()

        get_target_property(RAWSPEED_SYSTEM_INCLUDE_DIRECTORIES ${TARGET} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
        if(NOT "${RAWSPEED_SYSTEM_INCLUDE_DIRECTORIES}" MATCHES "NOTFOUND")
            set_target_properties(
                ${TARGET} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                                     "$<BUILD_INTERFACE:${RAWSPEED_SYSTEM_INCLUDE_DIRECTORIES}>"
            )
        endif()

        if(CXXIMG_ENABLE_INSTALL)
            install(TARGETS ${TARGET} EXPORT RawSpeedTargets)
        endif()
    endforeach(TARGET)

    if(CXXIMG_ENABLE_INSTALL)
        install(
            EXPORT RawSpeedTargets
            NAMESPACE rawspeed::
            DESTINATION cmake
        )
    endif()
endif()

# CXXOpts

if(CXXIMG_BUILD_TOOLS)
    FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY "https://github.com/jarro2783/cxxopts.git"
        GIT_TAG "63d1b65a694cfceafc20863afa75df49dfbe6b2a"
    )
    FetchContent_MakeAvailable(cxxopts)

    get_target_property(CXXOPTS_INCLUDE_DIRECTORIES cxxopts INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(cxxopts PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CXXOPTS_INCLUDE_DIRECTORIES}")
endif()

# Subdirectories

add_subdirectory(json_dto)
add_subdirectory(rapidjson)

if(HAVE_TIFF)
    add_subdirectory(tiff)
endif()
