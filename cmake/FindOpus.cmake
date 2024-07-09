if(UNIX)
    find_package(PkgConfig QUIET)
    pkg_check_modules(Opus QUIET opus)
endif()

if(NOT Opus_FOUND)
    find_path(Opus_INCLUDE_DIRS opus/opus.h)
    find_library(Opus_LIBRARY ogg)
else()
    set(Opus_LIBRARY ${pkgcfg_lib_Opus_opus})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus
        REQUIRED_VARS Opus_INCLUDE_DIRS Opus_LIBRARY
)

if(NOT TARGET Opus::opus)
    add_library(Opus::opus UNKNOWN IMPORTED)
    set_target_properties(Opus::opus PROPERTIES
            IMPORTED_LOCATION "${Opus_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Opus_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Opus_LIBRARY)
mark_as_advanced(Opus_INCLUDE_DIRS)
mark_as_advanced(Opus_FOUND)
