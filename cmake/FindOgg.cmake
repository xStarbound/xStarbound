if(UNIX)
    find_package(PkgConfig QUIET)
    pkg_check_modules(Ogg QUIET ogg)
endif()

if(NOT Ogg_FOUND)
    find_path(Ogg_INCLUDE_DIRS ogg/ogg.h)
    find_library(Ogg_LIBRARY ogg)
else()
    set(Ogg_LIBRARY ${pkgcfg_lib_Ogg_ogg})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg
        REQUIRED_VARS Ogg_INCLUDE_DIRS Ogg_LIBRARY
)

if(NOT TARGET Ogg::ogg)
    add_library(Ogg::ogg UNKNOWN IMPORTED)
    set_target_properties(Ogg::ogg PROPERTIES
            IMPORTED_LOCATION "${Ogg_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Ogg_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Ogg_LIBRARIES)
mark_as_advanced(Ogg_INCLUDE_DIRS)
mark_as_advanced(Ogg_FOUND)
