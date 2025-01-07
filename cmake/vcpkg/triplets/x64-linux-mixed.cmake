set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

if(PORT MATCHES "sdl2" OR PORT MATCHES "qtbase")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

# FezzedOne: Downstreamed this fix from oSB.
if(PORT MATCHES "libsystemd")
    set(VCPKG_C_FLAGS "-std=c11")
    set(VCPKG_CXX_FLAGS "-std=c11")
endif()

# FezzedOne: Also downstreamed this to ensure the Discord library gets linked dynamically.
if(PORT MATCHES "discord-")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_FIXUP_ELF_RPATH ON)
