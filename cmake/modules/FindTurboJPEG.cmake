include(FindPackageHandleStandardArgs)

find_path(TurboJPEG_INCLUDE_DIR turbojpeg.h)
find_library(TurboJPEG_LIBRARY NAMES turbojpeg)

find_package_handle_standard_args(
    TurboJPEG
    REQUIRED_VARS TurboJPEG_LIBRARY TurboJPEG_INCLUDE_DIR
    HANDLE_COMPONENTS
)

if(TurboJPEG_FOUND AND NOT TARGET TurboJPEG::TurboJPEG)
    add_library(TurboJPEG::TurboJPEG UNKNOWN IMPORTED)
    set_target_properties(TurboJPEG::TurboJPEG PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${TurboJPEG_INCLUDE_DIR}")
    set_target_properties(
        TurboJPEG::TurboJPEG PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${TurboJPEG_LIBRARY}"
    )
endif()

mark_as_advanced(TurboJPEG_LIBRARY TurboJPEG_INCLUDE_DIR)
