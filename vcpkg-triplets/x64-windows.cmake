set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/toolchains/windows.cmake")
set(ENV{CC} cl.exe)
set(ENV{CXX} cl.exe)
set(ENV{PATH} "$ENV{WINE_MSVC_PATH}:$ENV{PATH}")