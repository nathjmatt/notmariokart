# FindWiiUse.cmake
# Find the WiiUse library and headers
#
# This module defines the following variables:
#   WIIUSE_FOUND       - Set to true if the library and headers are found
#   WIIUSE_INCLUDE_DIR - The include directory for wiiuse
#   WIIUSE_LIBRARY     - The wiiuse library
#
# And creates an imported target:
#   WiiUse::wiiuse     - The imported wiiuse target

find_path(WIIUSE_INCLUDE_DIR
    NAMES wiiuse.h
    HINTS ${WIIUSE_ROOT}/include
    PATH_SUFFIXES include
)

find_library(WIIUSE_LIBRARY
    NAMES wiiuse
    HINTS ${WIIUSE_ROOT}/lib ${WIIUSE_ROOT}/lib64
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WiiUse
    REQUIRED_VARS WIIUSE_LIBRARY WIIUSE_INCLUDE_DIR
)

if(WIIUSE_FOUND AND NOT TARGET WiiUse::wiiuse)
    add_library(WiiUse::wiiuse UNKNOWN IMPORTED)
    set_target_properties(WiiUse::wiiuse PROPERTIES
        IMPORTED_LOCATION "${WIIUSE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WIIUSE_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(WIIUSE_INCLUDE_DIR WIIUSE_LIBRARY)
