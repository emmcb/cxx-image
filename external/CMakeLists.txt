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
        GIT_TAG "7f27122442ef49bf9188cf7439945a26897c96e8"
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

# CXXOpts

if(CXXIMG_BUILD_TOOLS)
    FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY "https://github.com/jarro2783/cxxopts.git"
        GIT_TAG "3bf268481da8208d171d8908e6491459de3651d7"
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
