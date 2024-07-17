if(UNIX)
    find_package(PkgConfig QUIET)
    pkg_check_modules(Vorbis QUIET vorbis)
    pkg_check_modules(VorbisFile QUIET vorbisfile)
endif()

if(NOT Vorbis_FOUND)
    find_path(Vorbis_INCLUDE_DIRS vorbis/codec.h)
    find_path(VorbisFile_INCLUDE_DIRS vorbis/vorbisfile.h)
    find_library(Vorbis_LIBRARY vorbis)
    find_library(VorbisFile_LIBRARY vorbisfile)
else()
    set(Vorbis_LIBRARY ${pkgcfg_lib_Vorbis_vorbis})
    set(VorbisFile_LIBRARY ${pkgcfg_lib_VorbisFile_vorbisfile})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis
        REQUIRED_VARS Vorbis_INCLUDE_DIRS Vorbis_LIBRARY VorbisFile_INCLUDE_DIRS VorbisFile_LIBRARY
)

if(NOT TARGET Vorbis::vorbis)
    add_library(Vorbis::vorbis UNKNOWN IMPORTED)
    set_target_properties(Vorbis::vorbis PROPERTIES
            IMPORTED_LOCATION "${Vorbis_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIRS}"
    )
endif()

if(NOT TARGET Vorbis::vorbisfile)
    add_library(Vorbis::vorbisfile UNKNOWN IMPORTED)
    set_target_properties(Vorbis::vorbisfile PROPERTIES
            IMPORTED_LOCATION "${VorbisFile_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${VorbisFile_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Vorbis_LIBRARIES)
mark_as_advanced(VorbisFile_LIBRARIES)
mark_as_advanced(Vorbis_INCLUDE_DIRS)
mark_as_advanced(VorbisFile_INCLUDE_DIRS)
mark_as_advanced(Vorbis_FOUND)
