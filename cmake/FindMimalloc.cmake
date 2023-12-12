# - Try to find mimalloc headers and libraries.
#
# Usage of this module as follows:
#
#     find_package(mimalloc)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  MIMALLOC_ROOT_DIR Set this variable to the root installation of
#                    mimalloc if the module has problems finding
#                    the proper installation path.
#
# Variables defined by this module:
#
#  MIMALLOC_FOUND             System has mimalloc libs/headers
#  MIMALLOC_LIBRARIES         The mimalloc library/libraries
#  MIMALLOC_INCLUDE_DIR       The location of mimalloc headers

find_path(MIMALLOC_ROOT_DIR
    NAMES include/mimalloc/mimalloc.h
)

find_library(MIMALLOC_LIBRARY
    NAMES mimalloc
    HINTS ${MIMALLOC_ROOT_DIR}/lib
)

find_path(MIMALLOC_INCLUDE_DIR
    NAMES mimalloc/mimalloc.h
    HINTS ${MIMALLOC_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mimalloc DEFAULT_MSG
    MIMALLOC_LIBRARY
    MIMALLOC_INCLUDE_DIR
)

mark_as_advanced(
    MIMALLOC_ROOT_DIR
    MIMALLOC_LIBRARY
    MIMALLOC_INCLUDE_DIR
)
