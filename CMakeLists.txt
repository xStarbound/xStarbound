cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Build options
include(CMakeDependentOption)
option(STAR_BUILD_GUI "Build GUI utilities and Client" ON)
cmake_dependent_option(STAR_BUILD_QT_TOOLS "Build GUI utilities. Requires Qt 6.7 or higher" OFF "STAR_BUILD_GUI" OFF)
cmake_dependent_option(STAR_ENABLE_STEAM_INTEGRATION "Use Steam platform services" OFF "STAR_BUILD_GUI" OFF)
cmake_dependent_option(STAR_ENABLE_DISCORD_INTEGRATION "Use Discord platform services" OFF "STAR_BUILD_GUI" OFF)

option(STAR_LUA_APICHECK "Use Lua API checks" OFF)
cmake_dependent_option(STAR_USE_JEMALLOC "Use jemalloc allocators" OFF "NOT STAR_USE_MIMALLOC" OFF)
cmake_dependent_option(STAR_USE_MIMALLOC "Use mi-malloc allocators" OFF "NOT STAR_USE_JEMALLOC" OFF)

option(BUILD_TESTING "Build unit and game tests (NOTE: game tests require all game asset packs to run)" OFF)
cmake_dependent_option(STAR_MEMORY_SANITIZER "Build executables with memory sanitizers enabled" OFF "BUILD_TESTING" OFF)

# Beware: This will NOT work when cross-compiling to an incompatible architecture (e.g. aarch64 on x86_64) or a different OS (e.g. MinGW)!
# In this case, asset_packer must be build for the HOST system architecture. We can't do that easily within CMake though
# as we do not know if the host system has the appropriate dependencies as asset_packer needs basically a full Starbound build.
option(PACKAGE_XSB_ASSETS "Adds a command to create a packed.pak from xsb-assets to the ALL target and installs it." OFF)

option(STAR_INSTALL_TOOLS "Install user and modding tools (asset_packer, planet_mapgen etc.)" ON)
option(STAR_INSTALL_EXTRA_TOOLS "Install extra development tools (benchmarks etc.)" OFF)

set(STAR_GAME_RESOURCE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE FILEPATH "Location of the game assets to be used for running inside the build dir. Defaults to project root.")

# Only request vcpkg dependencies if required to decrease build time
# VCPKG_MANIFEST_FEATURES must be set before calling PROJECT().
# See vcpkg.json to see which dependencies each feature pulls in.
if(STAR_BUILD_GUI)
    list(APPEND VCPKG_MANIFEST_FEATURES gui)
endif()

if(STAR_BUILD_QT_TOOLS)
    list(APPEND VCPKG_MANIFEST_FEATURES qttools)
endif()

if(STAR_USE_JEMALLOC)
    list(APPEND VCPKG_MANIFEST_FEATURES jemalloc)
endif()

if(STAR_USE_MIMALLOC)
    list(APPEND VCPKG_MANIFEST_FEATURES mimalloc)
endif()

if(BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES tests)
endif()

# Find CCache
find_program(CCACHE_PATH ccache)
if(CCACHE_PATH)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
    message(STATUS "Using CCache")
else()
    message(STATUS "Not using CCache")
endif()

project(xsb2)

set(CMAKE_CONFIGURATION_TYPES Debug RelWithAsserts RelWithDebInfo Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS "" CACHE STRING "" FORCE)
# Update the docstring on CMAKE_BUILD_TYPE to show what options we actually
# allow
# set (CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Choose the type of build, options are: Debug RelWithAsserts RelWithDebInfo Release" FORCE)

# Discover all the relevant environment / system information and place the
# result in STAR_* cmake variables.

# STAR_SOURCE_IDENTIFIER may be set to any string value
if(NOT DEFINED STAR_SOURCE_IDENTIFIER)
    include(GetGitRevisionDescription)
    get_git_head_revision(STAR_GIT_REFSPEC STAR_GIT_HASHVAR)
    set(STAR_SOURCE_IDENTIFIER "${STAR_GIT_HASHVAR}")
endif()

# Architecture identifier, like i386, x86_64 or ppc
if(NOT DEFINED STAR_ARCHITECTURE)
    include(TargetArch)
    target_architecture(STAR_ARCHITECTURE)
endif()

# Either TRUE or FALSE
if(NOT DEFINED STAR_LITTLE_ENDIAN)
    include(TestBigEndian)
    test_big_endian(BIGENDIAN)
    if(NOT BIGENDIAN)
        set(STAR_LITTLE_ENDIAN TRUE)
    else()
        set(STAR_LITTLE_ENDIAN FALSE)
    endif()
endif()

# System name, like windows, macos, linux, freebsd, or (generic) unix
if(NOT DEFINED STAR_SYSTEM)
    if(WIN32)
        set(STAR_SYSTEM "windows")
    elseif(APPLE AND ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(STAR_SYSTEM "macos")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(STAR_SYSTEM "linux")
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        set(STAR_SYSTEM "freebsd")
    elseif(UNIX)
        set(STAR_SYSTEM "unix")
    else()
        set(STAR_SYSTEM "unknown")
    endif()
endif()

if(NOT DEFINED STAR_SYSTEM_FAMILY)
    if(WIN32)
        set(STAR_SYSTEM_FAMILY "windows")
    elseif(UNIX)
        set(STAR_SYSTEM_FAMILY "unix")
    else()
        set(STAR_SYSTEM_FAMILY "unknown")
    endif()
endif()

# C/C++ compiler ID, like clang, gnu, or msvc
if(NOT DEFINED STAR_COMPILER)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID)
        message(FATAL_ERROR "C and C++ compiler id do not match, unsupported build configuration")
    endif()

    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(STAR_COMPILER "clang")
    elseif(CMAKE_COMPILER_IS_GNUC)
        set(STAR_COMPILER "gnu")
    elseif(MSVC)
        set(STAR_COMPILER "msvc")
    else()
        STRING(TOLOWER "${CMAKE_C_COMPILER_ID}" STAR_COMPILER)
    endif()
endif()

# Enable OPTIONs based on the discovered system / environment...
cmake_dependent_option(STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX "Statically link libgcc and libstdc++" OFF "STAR_COMPILER STREQUAL gnu" OFF)
cmake_dependent_option(STAR_ENABLE_GCC_PROFILING "Enable gcc/g++ profiling via the -pg flag" OFF "STAR_COMPILER STREQUAL gnu" OFF)
cmake_dependent_option(STAR_ENABLE_GLIBCXX_DEBUG "Enable _GLIBCXX_DEBUG for g++" OFF "STAR_COMPILER STREQUAL gnu" OFF)
cmake_dependent_option(STAR_ENABLE_STATIC_MSVC_RUNTIME "Statically link with the CRT" OFF "STAR_COMPILER STREQUAL msvc" OFF)

# FezzedOne: More MinGW stuff.
if(STAR_COMPILER STREQUAL "gnu" AND STAR_SYSTEM STREQUAL "windows" AND STAR_CROSS_COMPILE)
    message(STATUS "Cross-compiling for Windows on Linux")
    set(TARGET "x86_64-w64-mingw32")
    set(CMAKE_C_COMPILER_TARGET ${TARGET})
    set(CMAKE_CXX_COMPILER_TARGET ${TARGET})
    set(CMAKE_SYSTEM "Windows")
    set(CMAKE_SYSTEM_NAME "Windows")
    set(CMAKE_SYSTEM_VERSION "10")
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    set(MINGW ON)
    set(WIN32 ON)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32/ ${PROJECT_SOURCE_DIR}/../)
    set(CMAKE_PREFIX_PATH /usr/x86_64-w64-mingw32/ ${PROJECT_SOURCE_DIR}/../)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

# Report all the discovered system / environment settings and all options.

message(STATUS "Source ID: ${STAR_SOURCE_IDENTIFIER}")
message(STATUS "Architecture: ${STAR_ARCHITECTURE}")
message(STATUS "Little Endian: ${STAR_LITTLE_ENDIAN}")
message(STATUS "System: ${STAR_SYSTEM}")
message(STATUS "System family: ${STAR_SYSTEM_FAMILY}")
message(STATUS "C/C++ compiler: ${STAR_COMPILER}")
if(VCPKG_MANIFEST_MODE)
    message(STATUS "vcpkg: Building in Manifest mode, not using host system libraries.")
else()
    message(STATUS "vcpkg: Disabled, using host system libraries.")
endif()

if(DEFINED STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX)
    message(STATUS "Statically linking to libgcc / libstdc++: ${STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX}")
endif()

if(DEFINED STAR_ENABLE_STATIC_MSVC_RUNTIME)
    message(STATUS "Statically linking to CRT: ${STAR_ENABLE_STATIC_MSVC_RUNTIME}")
endif()

if(DEFINED STAR_ENABLE_GLIBCXX_DEBUG)
    message(STATUS "Enabling _GLIBCXX_DEBUG: ${STAR_ENABLE_GLIBCXX_DEBUG}")
endif()

message(STATUS "Building GUI: ${STAR_BUILD_GUI}")

if(DEFINED STAR_BUILD_QT_TOOLS)
    message(STATUS "Building Qt tools: ${STAR_BUILD_QT_TOOLS}")
endif()

if(DEFINED STAR_ENABLE_STEAM_INTEGRATION)
    message(STATUS "Using Steam platform services: ${STAR_ENABLE_STEAM_INTEGRATION}")
endif()

if(DEFINED STAR_ENABLE_DISCORD_INTEGRATION)
    message(STATUS "Using Discord platform services: ${STAR_ENABLE_DISCORD_INTEGRATION}")
endif()

message(STATUS "Using Lua API checks: ${STAR_LUA_APICHECK}")
message(STATUS "Using jemalloc: ${STAR_USE_JEMALLOC}")

# Set C defines and cmake variables based on the build settings we have now
# determined...

# Set a cmake variable to true and define a corresponding C/C++ definition
FUNCTION(set_flag flagValue)
    set(${flagValue} TRUE PARENT_SCOPE)
    add_definitions(-D${flagValue})
ENDFUNCTION()

if(STAR_LITTLE_ENDIAN)
    set_flag(STAR_LITTLE_ENDIAN)
elseif()
    set_flag(STAR_BIG_ENDIAN)
endif()

if(STAR_ARCHITECTURE STREQUAL "i386")
    set_flag(STAR_ARCHITECTURE_I386)
elseif(STAR_ARCHITECTURE STREQUAL "x86_64")
    set_flag(STAR_ARCHITECTURE_X86_64)
endif()

if(STAR_SYSTEM STREQUAL "windows")
    set_flag(STAR_SYSTEM_WINDOWS)
elseif(STAR_SYSTEM STREQUAL "macos")
    set_flag(STAR_SYSTEM_MACOS)
elseif(STAR_SYSTEM STREQUAL "linux")
    set_flag(STAR_SYSTEM_LINUX)
elseif(STAR_SYSTEM STREQUAL "freebsd")
    set_flag(STAR_SYSTEM_FREEBSD)
endif()

if(STAR_SYSTEM_FAMILY STREQUAL "windows")
    set_flag(STAR_SYSTEM_FAMILY_WINDOWS)
elseif(STAR_SYSTEM_FAMILY STREQUAL "unix")
    set_flag(STAR_SYSTEM_FAMILY_UNIX)
endif()

if(STAR_COMPILER STREQUAL "gnu")
    set_flag(STAR_COMPILER_GNU)
elseif(STAR_COMPILER STREQUAL "clang")
    set_flag(STAR_COMPILER_CLANG)
elseif(STAR_COMPILER STREQUAL "msvc")
    set_flag(STAR_COMPILER_MSVC)
endif()

if(STAR_LUA_APICHECK)
    add_definitions(-DLUA_USE_APICHECK)
endif()

if(STAR_SYSTEM_WINDOWS)
    # LUA_USE_WINDOWS is automatically defined in luaconf if _WIN32 is defined
elseif(STAR_SYSTEM_MACOS)
    add_definitions(-DLUA_USE_MACOSX)
elseif(STAR_SYSTEM_LINUX)
    add_definitions(-DLUA_USE_LINUX)
elseif(STAR_SYSTEM_FAMILY_UNIX)
    add_definitions(-DLUA_USE_POSIX)
endif()

if(STAR_ENABLE_STEAM_INTEGRATION)
    add_definitions(-DSTAR_ENABLE_STEAM_INTEGRATION)
endif()

if(STAR_ENABLE_DISCORD_INTEGRATION)
    add_definitions(-DSTAR_ENABLE_DISCORD_INTEGRATION)
endif()

if(STAR_USE_JEMALLOC)
    add_definitions(-DSTAR_USE_JEMALLOC)
endif()

# Set C/C++ compiler flags based on build environment...

if(STAR_COMPILER_GNU)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused -Wno-unqualified-std-cast-call -Wno-implicit-fallthrough -no-pie")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wno-unused -Wno-implicit-fallthrough -no-pie")

    if(STAR_SYSTEM_FAMILY_WINDOWS)
        # FezzedOne: Fixed GCC linker flag for Windows targets.
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--export-all-symbols -mthreads")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-all-symbols -mthreads")
        if(STAR_CROSS_COMPILE)
            # Cross-compiled binaries have to be statically linked to avoid weird "un-Windows-like" GCC DLL crap.
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -DSTAR_CROSS_COMPILE")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc -DSTAR_CROSS_COMPILE")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/x86_64-w64-mingw32/lib")
            if(STAR_USE_JEMALLOC)
            endif()
        endif()
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic -pthread -D_REENTRANT")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -pthread -D_REENTRANT")
    endif()

    if(STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc") # -static
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++") # -static
    endif()

    if(STAR_ENABLE_GCC_PROFILING)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    endif()

    set(CMAKE_C_FLAGS_DEBUG "-g -Og")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")

    # FezzedOne: Need to define `FLOAT_APPROX` for Opus whenever `-Ofast` is used.

    set(CMAKE_C_FLAGS_RELWITHASSERTS "-g -DFLOAT_APPROX -Ofast")
    set(CMAKE_CXX_FLAGS_RELWITHASSERTS "-g -DFLOAT_APPROX -Ofast")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -DFLOAT_APPROX -Ofast")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -DFLOAT_APPROX -Ofast")

    set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -DFLOAT_APPROX -Ofast")
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -DFLOAT_APPROX -Ofast")

    if(STAR_MEMORY_SANITIZER)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fsanitize=address")
    endif()

elseif(STAR_COMPILER_CLANG)
    if(STAR_SYSTEM_LINUX)
        # Stop Clang from bitching about all those thousands of `move` calls without an `std::` (or `Star::`, since there's an alias in that namespace).
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--export-dynamic -Wall -Wextra -Wuninitialized -Wno-unqualified-std-cast-call -Wno-parentheses-equality -Wno-deprecated-declarations")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wl,--export-dynamic -Wall -Wextra -Wuninitialized -Wno-parentheses-equality -Wno-deprecated-declarations")
    elseif(STAR_SYSTEM_MACOS)
        # Stop Clang from bitching about all those thousands of `move` calls without an `std::` (or `Star::`, since there's an alias in that namespace).
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-export_dynamic -Wall -Wextra -Wuninitialized -Wno-unqualified-std-cast-call -Wno-parentheses-equality -Wno-deprecated-declarations")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wl,-export_dynamic -Wall -Wextra -Wuninitialized -Wno-parentheses-equality -Wno-deprecated-declarations")
    endif()

    if(STAR_SYSTEM_MACOS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    elseif(STAR_SYSTEM_LINUX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
    elseif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie -pthread -D_REENTRANT")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -pthread -D_REENTRANT")
    endif()

    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")

    # FezzedOne: Need to define `FLOAT_APPROX` for Opus whenever `-Ofast` is used.

    set(CMAKE_C_FLAGS_RELWITHASSERTS "-g -O3") # -DFLOAT_APPROX -Ofast
    set(CMAKE_CXX_FLAGS_RELWITHASSERTS "-g O3") # -DFLOAT_APPROX -Ofast

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O3") # -DFLOAT_APPROX -Ofast
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O3") # -DFLOAT_APPROX -Ofast

    set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3") # -DFLOAT_APPROX -Ofast
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3") # -DFLOAT_APPROX -Ofast

    if(STAR_MEMORY_SANITIZER)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")
        set(CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fsanitize=address")
    endif()

elseif(STAR_COMPILER_MSVC)
    # /MP      - Multi-processor building
    # /EHsc    - Enable normal C++ exception handling
    # /bigobj  - More sections in .obj files (Cannot build in Debug without it)
    # /MT      - Use multi-threaded statically linked C runtime library
    # /GA      - Optimize for windows application
    # /Ox      - Full optimization
    # /fp:fast - Equivalent to -ffast-math
    # /GS-     - Disable buffers security check
    # /Zi      - Generates debugging information without Edit and Continue
    # /Gy      - Use function-level linking
    # /wd4996  - Disable warnings about unsafe C functions
    # /wd4351  - Disable warnings about new behavior of default initialization of
    #            arrays (which is the correct behavior anyway)
    # /wd4800  - Disable warnings about using non-bool as true or false (useless
    #            performance warning)
    # /wd4244  - Disable warnings about type conversion loss of data, it's a nice
    #            warning, but it triggers on lots and lots of harmless things that no
    #            other compiler warns about, like passing an int as a float parameter
    # /wd4305  - Disable warnings about truncation from double to float
    # /wd4267  - Disable warnings about 64 - 32 bit truncation
    # /wd4456  - Disable warnings about hiding previous local declaration
    # /wd4503  - Silence warnings about MSVC generating a name so long it has to
    #            truncate it
    # /wd4250  - Silence "XX inherits YY via dominance"
    # /wd4624  - Silence implicitly deleted destructor warnings that show up when
    #            using unions in interesting ways.

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /EHsc /bigobj /wd4996 /wd4351 /wd4800 /wd4244 /wd4305 /wd4267 /wd4456 /wd4503 /wd4250 /wd4624")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /MP /EHsc /bigobj /wd4996 /wd4351 /wd4800 /wd4244 /wd4305 /wd4267 /wd4456 /wd4503 /wd4250 /wd4624 /Zc:__cplusplus")

    if(STAR_ENABLE_STATIC_MSVC_RUNTIME)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
    endif()

    set(CMAKE_C_FLAGS_DEBUG "/Zi /Od")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")

    set(CMAKE_C_FLAGS_RELWITHASSERTS "/Ox /fp:fast /GA /GS- /Zi /Gy")
    set(CMAKE_CXX_FLAGS_RELWITHASSERTS "/Ox /fp:fast /GA /GS- /Zi /Gy")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /fp:fast /GA /GS- /Zi /Gy /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Ox /fp:fast /GA /GS- /Zi /Gy /DNDEBUG")

    set(CMAKE_C_FLAGS_RELEASE "/Ox /fp:fast /GA /GS- /Gy /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox /fp:fast /GA /GS- /Gy /DNDEBUG")

    if(STAR_ARCHITECTURE_I386)
        # Assume all 32 bit target cpus support MMX, SSE, and SSE2

        set(CMAKE_C_FLAGS_RELWITHASSERTS "${CMAKE_C_FLAGS_RELWITHASSERTS} /arch:SSE2")
        set(CMAKE_CXX_FLAGS_RELWITHASSERTS "${CMAKE_CXX_FLAGS_RELWITHASSERTS} /arch:SSE2")

        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /arch:SSE2")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /arch:SSE2")

        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE2")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2")
    endif()

    add_definitions(/DUNICODE)
    add_definitions(/D_UNICODE)
    add_definitions(/DNOMINMAX)

    if(STAR_MEMORY_SANITIZER)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /fsanitize=address")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /fsanitize=address")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /fsanitize=address")
    endif()

else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread -D_REENTRANT")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -pthread -D_REENTRANT")

    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")

    set(CMAKE_C_FLAGS_RELWITHASSERTS "-g -O2")
    set(CMAKE_CXX_FLAGS_RELWITHASSERTS "-g -O2")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")

    set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")

endif()

# Set other global build settings based on environment...
if(STAR_SYSTEM_MACOS)
    set(CMAKE_MODULE_LINKER_FLAGS "-flat_namespace -undefined suppress")
elseif(STAR_SYSTEM_WINDOWS)
    set(CMAKE_RC_COMPILER_INIT windres)

    enable_language(RC)
    if(STAR_COMPILER STREQUAL "msvc")
        set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> /fo <OBJECT> <SOURCE>")
    else()
        set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    endif()
endif()

if(STAR_COMPILER STREQUAL "msvc")
    # /largeaddressaware - Make 32 bit build able to use 3GB addresses
    # /OPT:REF           - Eliminates functions and data that are never referenced
    # /OPT:ICF           - Performs identical COMDAT folding
    # /PDBCompress       - Hint to windows that it should compress the resulting PDB files
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /largeaddressaware /OPT:REF /OPT:ICF /PDBCompress")

    # Make sure RelWithAsserts has debugging enabled
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS "${CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS} /DEBUG")
endif()

if(STAR_SYSTEM_WINDOWS)
    if(STAR_CROSS_COMPILE)
        set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lws2_32 -liphlpapi -lshlwapi -ldbghelp")
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lws2_32 -liphlpapi -lshlwapi -ldbghelp")
    else()
        set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ws2_32.lib iphlpapi.lib shlwapi.lib dbghelp.lib")
        set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ws2_32.lib iphlpapi.lib shlwapi.lib dbghelp.lib")
    endif()

elseif(STAR_SYSTEM_LINUX)
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread -ldl -lrt")
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lpthread -ldl -lrt")

elseif(STAR_SYSTEM_FREEBSD)
    set(CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread -lrt")
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lpthread -lrt")

endif()

# Find all required external libraries, based on build settings...
if(STAR_USE_JEMALLOC AND STAR_USE_MIMALLOC)
    message(FATAL_ERROR "Please only specify ONE memory allocator to be used: either STAR_USE_JEMALLOC or STAR_USE_MIMALLOC.")
endif()

# Need to set install dirs before finding Qt.
include(Install.cmake)

if(STAR_USE_JEMALLOC)
    # Assumes jemalloc was configured with a "je_" function prefix
    find_package(JeMalloc REQUIRED)

    include_directories(SYSTEM ${JEMALLOC_INCLUDE_DIR})
    set(STAR_EXT_LIBS ${JEMALLOC_LIBRARY})
endif()

if(STAR_USE_MIMALLOC)
    # Assumes mimalloc was configured with an "mi_" function prefix
    find_package(mimalloc REQUIRED)

    include_directories(SYSTEM ${MIMALLOC_INCLUDE_DIR})
    set(STAR_EXT_LIBS ${MIMALLOC_LIBRARY})
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
# Prefer CMake config package. Some distros may only install the pkgconfig file though.
find_package(Ogg CONFIG QUIET)
find_package(Vorbis CONFIG QUIET)
find_package(Opus CONFIG QUIET)
if(NOT TARGET Ogg::ogg)
    find_package(Ogg MODULE REQUIRED)
endif()
if(NOT TARGET Vorbis::vorbis)
    find_package(Vorbis MODULE REQUIRED)
endif()
if(NOT TARGET Opus::opus)
    find_package(Opus MODULE REQUIRED)
endif()

if(STAR_CROSS_COMPILE)
    include_directories(SYSTEM
            ${ZLIB_INCLUDE_DIR}
            ${PNG_INCLUDE_DIR}
            ${FREETYPE_INCLUDE_DIRS}
            ${OGGVORBIS_INCLUDE_DIR}
    )

    set(STAR_EXT_LIBS ${STAR_EXT_LIBS}
            Vorbis::vorbisfile
            Vorbis::vorbis
            Opus::opus
            Ogg::ogg
            Freetype::Freetype
            "/usr/x86_64-w64-mingw32/lib/libbrotlidec.a"
            "/usr/x86_64-w64-mingw32/lib/libbrotlienc.a"
            "/usr/x86_64-w64-mingw32/lib/libbrotlicommon.a"
            "/usr/x86_64-w64-mingw32/lib/libbz2.a"
            PNG::PNG
            ZLIB::ZLIB
    )
else()

    set(STAR_EXT_LIBS ${STAR_EXT_LIBS}
            Vorbis::vorbisfile
            Vorbis::vorbis
            Ogg::ogg
            Freetype::Freetype
            PNG::PNG
            ZLIB::ZLIB
            Opus::opus
    )
endif()

if(STAR_BUILD_GUI)
    find_package(SDL2 CONFIG REQUIRED)
    include_directories(SYSTEM ${SDL2_INCLUDE_DIR})
    if(STAR_CROSS_COMPILE)
        set(STAR_EXT_GUI_LIBS "${PROJECT_SOURCE_DIR}/../lib/mingw/SDL2.dll" "${PROJECT_SOURCE_DIR}/../lib/mingw/glew32.dll")
    else()
        # if (STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX AND STAR_SYSTEM_LINUX AND STAR_COMPILER_GNU)
        #   # SDL2 should be dynamically linked.
        #   set (STAR_EXT_GUI_LIBS "-Wl,-Bdynamic" ${SDL2_LIBRARY})
        # else()
        set(STAR_EXT_GUI_LIBS $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
        # endif()
    endif()

    find_package(OpenGL REQUIRED)
    find_package(GLEW CONFIG)
    if(NOT TARGET GLEW::GLEW)
        find_package(GLEW MODULE REQUIRED)
    endif()

    set(STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} ${OPENGL_LIBRARY} GLEW::GLEW)

    if(STAR_BUILD_QT_TOOLS)
        find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)
        qt_standard_project_setup()
    endif()
endif()

# Set basic build flags, include all the relevant source directories, based on
# build settings...

set(BUILD_SHARED_LIBS false)

add_subdirectory(source)

include(PackXSBAssets.cmake)

if(BUILD_TESTING)
    # Needs to reside at the top-level CMakeLists.txt!
    enable_testing()
    add_subdirectory(source/test)
endif()

