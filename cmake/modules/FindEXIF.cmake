include(FindPackageHandleStandardArgs)

find_path(EXIF_INCLUDE_DIR libexif/exif-data.h)
find_library(EXIF_LIBRARY NAMES exif)

find_package_handle_standard_args(
    EXIF
    REQUIRED_VARS EXIF_LIBRARY EXIF_INCLUDE_DIR
    HANDLE_COMPONENTS
)

if(EXIF_FOUND AND NOT TARGET EXIF::EXIF)
    add_library(EXIF::EXIF UNKNOWN IMPORTED)
    set_target_properties(EXIF::EXIF PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EXIF_INCLUDE_DIR}")
    set_target_properties(
        EXIF::EXIF PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${EXIF_LIBRARY}"
    )
endif()

mark_as_advanced(EXIF_LIBRARY EXIF_INCLUDE_DIR)
