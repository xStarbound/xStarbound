CMAKE_MINIMUM_REQUIRED (VERSION 3.16)

# Find CCache
FIND_PROGRAM(CCACHE_PATH ccache)
IF (CCACHE_PATH)
  SET (CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PATH}")
  SET (CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
  MESSAGE (STATUS "Using CCache")
ELSE ()
  MESSAGE (STATUS "Not using CCache")
ENDIF ()


PROJECT (xsb2)
SET (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake)

SET (CMAKE_CONFIGURATION_TYPES Debug RelWithAsserts RelWithDebInfo Release)
SET (CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
SET (CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS "" CACHE STRING "" FORCE)
# Update the docstring on CMAKE_BUILD_TYPE to show what options we actually
# allow
# SET (CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Choose the type of build, options are: Debug RelWithAsserts RelWithDebInfo Release" FORCE)

# Discover all the relevant environment / system information and place the
# result in STAR_* cmake variables.

# STAR_SOURCE_IDENTIFIER may be set to any string value
IF (NOT DEFINED STAR_SOURCE_IDENTIFIER)
  INCLUDE (GetGitRevisionDescription)
  GET_GIT_HEAD_REVISION (STAR_GIT_REFSPEC STAR_GIT_HASHVAR)
  SET (STAR_SOURCE_IDENTIFIER "${STAR_GIT_HASHVAR}")
ENDIF ()

# Architecture identifier, like i386, x86_64 or ppc
IF (NOT DEFINED STAR_ARCHITECTURE)
  INCLUDE (TargetArch)
  TARGET_ARCHITECTURE (STAR_ARCHITECTURE)
ENDIF ()

# Either TRUE or FALSE
IF (NOT DEFINED STAR_LITTLE_ENDIAN)
  INCLUDE (TestBigEndian)
  TEST_BIG_ENDIAN (BIGENDIAN)
  IF (NOT BIGENDIAN)
    SET (STAR_LITTLE_ENDIAN TRUE)
  ELSE ()
    SET (STAR_LITTLE_ENDIAN FALSE)
  ENDIF ()
ENDIF ()

# System name, like windows, macos, linux, freebsd, or (generic) unix
IF (NOT DEFINED STAR_SYSTEM)
  IF (WIN32)
    SET (STAR_SYSTEM "windows")
  ELSEIF (APPLE AND ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET (STAR_SYSTEM "macos")
  ELSEIF (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET (STAR_SYSTEM "linux")
  ELSEIF (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    SET (STAR_SYSTEM "freebsd")
  ELSEIF (UNIX)
    SET (STAR_SYSTEM "unix")
  ELSE ()
    SET (STAR_SYSTEM "unknown")
  ENDIF ()
ENDIF ()

IF (NOT DEFINED STAR_SYSTEM_FAMILY)
  IF (WIN32)
    SET (STAR_SYSTEM_FAMILY "windows")
  ELSEIF (UNIX)
    SET (STAR_SYSTEM_FAMILY "unix")
  ELSE ()
    SET (STAR_SYSTEM_FAMILY "unknown")
  ENDIF ()
ENDIF ()

# C/C++ compiler ID, like clang, gnu, or msvc
IF (NOT DEFINED STAR_COMPILER)
  IF (NOT CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID)
    MESSAGE (FATAL_ERROR "C and C++ compiler id do not match, unsupported build configuration")
  ENDIF ()

  IF (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    SET (STAR_COMPILER "clang")
  ELSEIF (CMAKE_COMPILER_IS_GNUC)
    SET (STAR_COMPILER "gnu")
  ELSEIF (MSVC)
    SET (STAR_COMPILER "msvc")
  ELSE ()
    STRING (TOLOWER "${CMAKE_C_COMPILER_ID}" STAR_COMPILER)
  ENDIF ()
ENDIF ()

# Enable OPTIONs based on the discovered system / environment...

IF (STAR_COMPILER STREQUAL "gnu")
  OPTION (STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX "Statically link libgcc and libstdc++" OFF)
  OPTION (STAR_ENABLE_GCC_PROFILING "Enable gcc/g++ profiling via the -pg flag" OFF)
  OPTION (STAR_ENABLE_GLIBCXX_DEBUG "Enable _GLIBCXX_DEBUG for g++" OFF)
ENDIF ()

IF (STAR_COMPILER STREQUAL "msvc")
  OPTION (STAR_ENABLE_STATIC_MSVC_RUNTIME "Statically link with the CRT" OFF)
ENDIF ()

# FezzedOne: More MinGW stuff.
IF (STAR_COMPILER STREQUAL "gnu" AND STAR_SYSTEM STREQUAL "windows" AND STAR_CROSS_COMPILE)
  MESSAGE (STATUS "Cross-compiling for Windows on Linux")
  SET (TARGET "x86_64-w64-mingw32")
  SET (CMAKE_C_COMPILER_TARGET ${TARGET})
  SET (CMAKE_CXX_COMPILER_TARGET ${TARGET})
  SET (CMAKE_SYSTEM "Windows")
  SET (CMAKE_SYSTEM_NAME "Windows")
  SET (CMAKE_SYSTEM_VERSION "10")
  SET (CMAKE_SYSTEM_PROCESSOR "x86_64")
  SET (MINGW ON)
  SET (WIN32 ON)
  SET (CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  SET (CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32/ ${PROJECT_SOURCE_DIR}/../)
  SET (CMAKE_PREFIX_PATH /usr/x86_64-w64-mingw32/ ${PROJECT_SOURCE_DIR}/../)
  SET (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  SET (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  SET (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
ENDIF()

OPTION (STAR_BUILD_GUI "Build GUI utilities and Client" ON)

IF (STAR_BUILD_GUI)
  OPTION (STAR_BUILD_QT_TOOLS "Build GUI utilities that require Qt" OFF)
  OPTION (STAR_ENABLE_STEAM_INTEGRATION "Use Steam platform services" OFF)
  OPTION (STAR_ENABLE_DISCORD_INTEGRATION "Use Discord platform services" OFF)
ENDIF ()

OPTION (STAR_LUA_APICHECK "Use lua api checks" OFF)
OPTION (STAR_USE_JEMALLOC "Use jemalloc allocators" OFF)

# Report all the discovered system / environment settings and all options.

MESSAGE (STATUS "Source ID: ${STAR_SOURCE_IDENTIFIER}")
MESSAGE (STATUS "Architecture: ${STAR_ARCHITECTURE}")
MESSAGE (STATUS "Little Endian: ${STAR_LITTLE_ENDIAN}")
MESSAGE (STATUS "System: ${STAR_SYSTEM}")
MESSAGE (STATUS "System family: ${STAR_SYSTEM_FAMILY}")
MESSAGE (STATUS "C/C++ compiler: ${STAR_COMPILER}")

IF (DEFINED STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX)
  MESSAGE (STATUS "Statically linking to libgcc / libstdc++: ${STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX}")
ENDIF ()

IF (DEFINED STAR_ENABLE_STATIC_MSVC_RUNTIME)
  MESSAGE (STATUS "Statically linking to CRT: ${STAR_ENABLE_STATIC_MSVC_RUNTIME}")
ENDIF ()

IF (DEFINED STAR_ENABLE_GLIBCXX_DEBUG)
  MESSAGE (STATUS "Enabling _GLIBCXX_DEBUG: ${STAR_ENABLE_GLIBCXX_DEBUG}")
ENDIF ()

MESSAGE (STATUS "Building GUI: ${STAR_BUILD_GUI}")

IF (DEFINED STAR_BUILD_QT_TOOLS)
  MESSAGE (STATUS "Building Qt tools: ${STAR_BUILD_QT_TOOLS}")
ENDIF ()

IF (DEFINED STAR_ENABLE_STEAM_INTEGRATION)
  MESSAGE (STATUS "Using Steam platform services: ${STAR_ENABLE_STEAM_INTEGRATION}")
ENDIF ()

IF (DEFINED STAR_ENABLE_DISCORD_INTEGRATION)
  MESSAGE (STATUS "Using Discord platform services: ${STAR_ENABLE_DISCORD_INTEGRATION}")
ENDIF ()

MESSAGE (STATUS "Using Lua API checks: ${STAR_LUA_APICHECK}")
MESSAGE (STATUS "Using jemalloc: ${STAR_USE_JEMALLOC}")

# Set C defines and cmake variables based on the build settings we have now
# determined...

# Set a cmake variable to true and define a corresponding C/C++ definition
FUNCTION (SET_FLAG flagValue)
  SET (${flagValue} TRUE PARENT_SCOPE)
  ADD_DEFINITIONS (-D${flagValue})
ENDFUNCTION ()

IF (STAR_LITTLE_ENDIAN)
  SET_FLAG (STAR_LITTLE_ENDIAN)
ELSEIF ()
  SET_FLAG (STAR_BIG_ENDIAN)
ENDIF ()

IF (STAR_ARCHITECTURE STREQUAL "i386")
  SET_FLAG (STAR_ARCHITECTURE_I386)
ELSEIF (STAR_ARCHITECTURE STREQUAL "x86_64")
  SET_FLAG (STAR_ARCHITECTURE_X86_64)
ENDIF ()

IF (STAR_SYSTEM STREQUAL "windows")
  SET_FLAG (STAR_SYSTEM_WINDOWS)
ELSEIF (STAR_SYSTEM STREQUAL "macos")
  SET_FLAG (STAR_SYSTEM_MACOS)
ELSEIF (STAR_SYSTEM STREQUAL "linux")
  SET_FLAG (STAR_SYSTEM_LINUX)
ELSEIF (STAR_SYSTEM STREQUAL "freebsd")
  SET_FLAG (STAR_SYSTEM_FREEBSD)
ENDIF ()

IF (STAR_SYSTEM_FAMILY STREQUAL "windows")
  SET_FLAG (STAR_SYSTEM_FAMILY_WINDOWS)
ELSEIF (STAR_SYSTEM_FAMILY STREQUAL "unix")
  SET_FLAG (STAR_SYSTEM_FAMILY_UNIX)
ENDIF ()

IF (STAR_COMPILER STREQUAL "gnu")
  SET_FLAG (STAR_COMPILER_GNU)
ELSEIF (STAR_COMPILER STREQUAL "clang")
  SET_FLAG (STAR_COMPILER_CLANG)
ELSEIF (STAR_COMPILER STREQUAL "msvc")
  SET_FLAG (STAR_COMPILER_MSVC)
ENDIF ()

IF (STAR_LUA_APICHECK)
  ADD_DEFINITIONS (-DLUA_USE_APICHECK)
ENDIF ()

IF (STAR_SYSTEM_WINDOWS)
  # LUA_USE_WINDOWS is automatically defined in luaconf if _WIN32 is defined
ELSEIF (STAR_SYSTEM_MACOS)
  ADD_DEFINITIONS(-DLUA_USE_MACOSX)
ELSEIF (STAR_SYSTEM_LINUX)
  ADD_DEFINITIONS(-DLUA_USE_LINUX)
ELSEIF (STAR_SYSTEM_FAMILY_UNIX)
  ADD_DEFINITIONS(-DLUA_USE_POSIX)
ENDIF ()

IF (STAR_ENABLE_STEAM_INTEGRATION)
  ADD_DEFINITIONS (-DSTAR_ENABLE_STEAM_INTEGRATION)
ENDIF ()

IF (STAR_ENABLE_DISCORD_INTEGRATION)
  ADD_DEFINITIONS (-DSTAR_ENABLE_DISCORD_INTEGRATION)
ENDIF ()

IF (STAR_USE_JEMALLOC)
  ADD_DEFINITIONS (-DSTAR_USE_JEMALLOC)
ENDIF ()

# Set C/C++ compiler flags based on build environment...

IF (STAR_COMPILER_GNU)
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused -Wno-unqualified-std-cast-call -Wno-implicit-fallthrough -no-pie")
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wno-unused -Wno-implicit-fallthrough -no-pie")

  IF (STAR_SYSTEM_FAMILY_WINDOWS)
    # FezzedOne: Fixed GCC linker flag for Windows targets.
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--export-all-symbols -mthreads")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-all-symbols -mthreads")
    IF (STAR_CROSS_COMPILE)
      # Cross-compiled binaries have to be statically linked to avoid weird "un-Windows-like" GCC DLL crap.
      SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -DSTAR_CROSS_COMPILE")
      SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc -DSTAR_CROSS_COMPILE")
      SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/x86_64-w64-mingw32/lib")
      IF (STAR_USE_JEMALLOC)
      ENDIF ()
    ENDIF ()
  ELSE ()
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -rdynamic -pthread -D_REENTRANT")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -pthread -D_REENTRANT")
  ENDIF ()

  IF (STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc") # -static
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++") # -static
  ENDIF ()

  IF (STAR_ENABLE_GCC_PROFILING)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
  ENDIF ()

  SET (CMAKE_C_FLAGS_DEBUG "-g -Og -fsanitize=address")
  SET (CMAKE_CXX_FLAGS_DEBUG "-g -Og -fsanitize=address")
  set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

  # FezzedOne: Need to define `FLOAT_APPROX` for Opus whenever `-Ofast` is used.

  SET (CMAKE_C_FLAGS_RELWITHASSERTS "-g -DFLOAT_APPROX -Ofast")
  SET (CMAKE_CXX_FLAGS_RELWITHASSERTS "-g -DFLOAT_APPROX -Ofast")

  SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -DFLOAT_APPROX -Ofast -fsanitize=address")
  SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -DFLOAT_APPROX -Ofast -fsanitize=address")
  set (CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

  SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -DFLOAT_APPROX -Ofast")
  SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -DFLOAT_APPROX -Ofast")

ELSEIF (STAR_COMPILER_CLANG)
  IF (STAR_SYSTEM_LINUX)
    # Stop Clang from bitching about all those thousands of `move` calls without an `std::` (or `Star::`, since there's an alias in that namespace).
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--export-dynamic -Wall -Wextra -Wuninitialized -Wno-unqualified-std-cast-call -Wno-parentheses-equality -Wno-deprecated-declarations")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wl,--export-dynamic -Wall -Wextra -Wuninitialized -Wno-parentheses-equality -Wno-deprecated-declarations")
  ELSEIF (STAR_SYSTEM_MACOS)
    # Stop Clang from bitching about all those thousands of `move` calls without an `std::` (or `Star::`, since there's an alias in that namespace).
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-export_dynamic -Wall -Wextra -Wuninitialized -Wno-unqualified-std-cast-call -Wno-parentheses-equality -Wno-deprecated-declarations")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wl,-export_dynamic -Wall -Wextra -Wuninitialized -Wno-parentheses-equality -Wno-deprecated-declarations")
  ENDIF()

  IF (STAR_SYSTEM_MACOS)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    SET (CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
    SET (CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  ELSEIF (STAR_SYSTEM_LINUX)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
  ELSEIF ()
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie -pthread -D_REENTRANT")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -pthread -D_REENTRANT")
  ENDIF ()

  SET (CMAKE_C_FLAGS_DEBUG "-g")
  SET (CMAKE_CXX_FLAGS_DEBUG "-g")

  # FezzedOne: Need to define `FLOAT_APPROX` for Opus whenever `-Ofast` is used.

  SET (CMAKE_C_FLAGS_RELWITHASSERTS "-g -O3") # -DFLOAT_APPROX -Ofast
  SET (CMAKE_CXX_FLAGS_RELWITHASSERTS "-g O3") # -DFLOAT_APPROX -Ofast

  SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O3") # -DFLOAT_APPROX -Ofast
  SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O3") # -DFLOAT_APPROX -Ofast

  SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3") # -DFLOAT_APPROX -Ofast
  SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3") # -DFLOAT_APPROX -Ofast

ELSEIF (STAR_COMPILER_MSVC)
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

  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /EHsc /bigobj /wd4996 /wd4351 /wd4800 /wd4244 /wd4305 /wd4267 /wd4456 /wd4503 /wd4250 /wd4624")
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /MP /EHsc /bigobj /wd4996 /wd4351 /wd4800 /wd4244 /wd4305 /wd4267 /wd4456 /wd4503 /wd4250 /wd4624 /Zc:__cplusplus")

  IF (STAR_ENABLE_STATIC_MSVC_RUNTIME)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
  ENDIF ()

  SET (CMAKE_C_FLAGS_DEBUG "/Zi /Od")
  SET (CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")

  SET (CMAKE_C_FLAGS_RELWITHASSERTS "/Ox /fp:fast /GA /GS- /Zi /Gy")
  SET (CMAKE_CXX_FLAGS_RELWITHASSERTS "/Ox /fp:fast /GA /GS- /Zi /Gy")

  SET (CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /fp:fast /GA /GS- /Zi /Gy /DNDEBUG")
  SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Ox /fp:fast /GA /GS- /Zi /Gy /DNDEBUG")

  SET (CMAKE_C_FLAGS_RELEASE "/Ox /fp:fast /GA /GS- /Gy /DNDEBUG")
  SET (CMAKE_CXX_FLAGS_RELEASE "/Ox /fp:fast /GA /GS- /Gy /DNDEBUG")

  IF (STAR_ARCHITECTURE_I386)
    # Assume all 32 bit target cpus support MMX, SSE, and SSE2

    SET (CMAKE_C_FLAGS_RELWITHASSERTS "${CMAKE_C_FLAGS_RELWITHASSERTS} /arch:SSE2")
    SET (CMAKE_CXX_FLAGS_RELWITHASSERTS "${CMAKE_CXX_FLAGS_RELWITHASSERTS} /arch:SSE2")

    SET (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /arch:SSE2")
    SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /arch:SSE2")

    SET (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE2")
    SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2")
  ENDIF ()

  ADD_DEFINITIONS (/DUNICODE)
  ADD_DEFINITIONS (/D_UNICODE)
  ADD_DEFINITIONS (/DNOMINMAX)

ELSE ()
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread -D_REENTRANT")
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -pthread -D_REENTRANT")

  SET (CMAKE_C_FLAGS_DEBUG "-g")
  SET (CMAKE_CXX_FLAGS_DEBUG "-g")

  SET (CMAKE_C_FLAGS_RELWITHASSERTS "-g -O2")
  SET (CMAKE_CXX_FLAGS_RELWITHASSERTS "-g -O2")

  SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")
  SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")

  SET (CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
  SET (CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")

ENDIF ()

# Set other global build settings based on environment...

# IF (STAR_SYSTEM_LINUX AND STAR_COMPILER_CLANG)
#   SET (CMAKE_MODULE_LINKER_FLAGS "-no-pie")
# ENDIF ()

IF (STAR_SYSTEM_MACOS)
  SET (CMAKE_MODULE_LINKER_FLAGS "-flat_namespace -undefined suppress")
ELSEIF (STAR_SYSTEM_WINDOWS)
  SET (CMAKE_RC_COMPILER_INIT windres)

  ENABLE_LANGUAGE (RC)
  IF (STAR_COMPILER STREQUAL "msvc")
    SET (CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> /fo <OBJECT> <SOURCE>")
  ELSE ()
    SET (CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  ENDIF()
ENDIF ()

IF (STAR_COMPILER STREQUAL "msvc")
  # /largeaddressaware - Make 32 bit build able to use 3GB addresses
  # /OPT:REF           - Eliminates functions and data that are never referenced
  # /OPT:ICF           - Performs identical COMDAT folding
  # /PDBCompress       - Hint to windows that it should compress the resulting PDB files
  SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /largeaddressaware /OPT:REF /OPT:ICF /PDBCompress")

  # Make sure RelWithAsserts has debugging enabled
  SET (CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS "${CMAKE_EXE_LINKER_FLAGS_RELWITHASSERTS} /DEBUG")
ENDIF ()

IF (STAR_SYSTEM_WINDOWS)
  IF (STAR_CROSS_COMPILE)
    SET (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lws2_32 -liphlpapi -lshlwapi -ldbghelp")
    SET (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lws2_32 -liphlpapi -lshlwapi -ldbghelp")
  ELSE ()
    SET (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} ws2_32.lib iphlpapi.lib shlwapi.lib dbghelp.lib")
    SET (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ws2_32.lib iphlpapi.lib shlwapi.lib dbghelp.lib")
  ENDIF()

ELSEIF (STAR_SYSTEM_LINUX)
  SET (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread -ldl -lrt")
  SET (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lpthread -ldl -lrt")

ELSEIF (STAR_SYSTEM_FREEBSD)
  SET (CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES} -lpthread -lrt")
  SET (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lpthread -lrt")

ENDIF ()

# Find all required external libraries, based on build settings...

IF (STAR_USE_JEMALLOC)
  # Assumes jemalloc was configured with a "je_" function prefix
  FIND_PACKAGE (JeMalloc REQUIRED)

  INCLUDE_DIRECTORIES (SYSTEM ${JEMALLOC_INCLUDE_DIR})
  SET (STAR_EXT_LIBS ${JEMALLOC_LIBRARY})
ENDIF ()

IF (STAR_USE_MIMALLOC)
  # Assumes mimalloc was configured with an "mi_" function prefix
  FIND_PACKAGE(mimalloc REQUIRED)

  INCLUDE_DIRECTORIES (SYSTEM ${MIMALLOC_INCLUDE_DIR})
  SET (STAR_EXT_LIBS ${MIMALLOC_LIBRARY})
ENDIF()

FIND_PACKAGE (ZLIB REQUIRED)
FIND_PACKAGE (PNG REQUIRED)
FIND_PACKAGE (Freetype REQUIRED)
FIND_PACKAGE (OggVorbis REQUIRED)

IF (STAR_CROSS_COMPILE)
  INCLUDE_DIRECTORIES (SYSTEM
    ${ZLIB_INCLUDE_DIR}
    ${PNG_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${OGGVORBIS_INCLUDE_DIR}
  )

  SET (STAR_EXT_LIBS ${STAR_EXT_LIBS}
      ${VORBISFILE_LIBRARY}
      ${VORBIS_LIBRARY}
      ${OGG_LIBRARY}
      ${FREETYPE_LIBRARY}
      "/usr/x86_64-w64-mingw32/lib/libbrotlidec.a"
      "/usr/x86_64-w64-mingw32/lib/libbrotlienc.a"
      "/usr/x86_64-w64-mingw32/lib/libbrotlicommon.a"
      "/usr/x86_64-w64-mingw32/lib/libbz2.a"
      ${PNG_LIBRARY}
      ${ZLIB_LIBRARY}
      opus
  )
ELSE()
  INCLUDE_DIRECTORIES (SYSTEM
      ${ZLIB_INCLUDE_DIR}
      ${PNG_INCLUDE_DIR}
      ${FREETYPE_INCLUDE_DIRS}
      ${OGGVORBIS_INCLUDE_DIR}
  )

  SET (STAR_EXT_LIBS ${STAR_EXT_LIBS}
      ${VORBISFILE_LIBRARY}
      ${VORBIS_LIBRARY}
      ${OGG_LIBRARY}
      ${FREETYPE_LIBRARY}
      ${PNG_LIBRARY}
      ${ZLIB_LIBRARY}
      opus
  )
ENDIF()

IF (STAR_BUILD_GUI)
  FIND_PACKAGE (SDL2 REQUIRED)
  INCLUDE_DIRECTORIES (SYSTEM ${SDL2_INCLUDE_DIR})
  IF (STAR_CROSS_COMPILE)
    SET (STAR_EXT_GUI_LIBS "${PROJECT_SOURCE_DIR}/../lib/mingw/SDL2.dll" "${PROJECT_SOURCE_DIR}/../lib/mingw/glew32.dll")
  ELSE()
    # IF (STAR_ENABLE_STATIC_LIBGCC_LIBSTDCXX AND STAR_SYSTEM_LINUX AND STAR_COMPILER_GNU)
    #   # SDL2 should be dynamically linked.
    #   SET (STAR_EXT_GUI_LIBS "-Wl,-Bdynamic" ${SDL2_LIBRARY})
    # ELSE()
      SET (STAR_EXT_GUI_LIBS ${SDL2_LIBRARY})
    # ENDIF()
  ENDIF()

  FIND_PACKAGE (OpenGL REQUIRED)
  FIND_PACKAGE (GLEW REQUIRED)

  INCLUDE_DIRECTORIES (SYSTEM ${GLEW_INCLUDE_DIR} ${SDL2_INCLUDE_DIR})
  SET (STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

  IF (STAR_ENABLE_STEAM_INTEGRATION)
    FIND_PACKAGE (SteamApi REQUIRED)

    INCLUDE_DIRECTORIES (SYSTEM ${STEAM_API_INCLUDE_DIR})
    IF (STAR_CROSS_COMPILE)
      SET (STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} "${PROJECT_SOURCE_DIR}/../lib/mingw/steam_api64.dll")
    ELSE()
      SET (STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} ${STEAM_API_LIBRARY})
    ENDIF()
  ENDIF ()

  IF (STAR_ENABLE_DISCORD_INTEGRATION)
    FIND_PACKAGE (DiscordApi REQUIRED)

    INCLUDE_DIRECTORIES (SYSTEM ${DISCORD_API_INCLUDE_DIR})
    IF (STAR_CROSS_COMPILE)
      SET (STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} "${PROJECT_SOURCE_DIR}/../lib/mingw/discord_game_sdk.dll")
    ELSE()
      SET (STAR_EXT_GUI_LIBS ${STAR_EXT_GUI_LIBS} ${DISCORD_API_LIBRARY})
    ENDIF()
  ENDIF ()
ENDIF ()

# Set basic build flags, include all the relevant source directories, based on
# build settings...

SET (BUILD_SHARED_LIBS false)

# FezzedOne: MinGW setup. Windows executables built with GCC (presumably MinGW)
# go in `$src/dist-windows` or `$src/dist-windows-debug`, while all other executables,
# including Windows executables built with MinGW and Linux executables built with GCC,
# go in `$src/dist` or `$src/dist-debug`.
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
  IF (STAR_CROSS_COMPILE)
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist-windows-debug)
  ELSE()
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist-debug)
  ENDIF()
ELSEIF (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  IF (STAR_CROSS_COMPILE)
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist-windows-debug)
  ELSE()
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist-debug)
  ENDIF()
ELSE()
  IF (STAR_CROSS_COMPILE)
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist-windows)
  ELSE()
    # First set output dir for the generic no-config case (e.g. macos / linux)
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../dist)
  ENDIF()
ENDIF()

# Second, set output dir for multi-config builds (e.g. msvc)
FOREACH (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  STRING (TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    IF (STAR_CROSS_COMPILE)
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist-windows-debug)
    ELSE()
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist-debug)
    ENDIF()
  ELSEIF (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    IF (STAR_CROSS_COMPILE)
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist-windows-debug)
    ELSE()
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist-debug)
    ENDIF()
  ELSE()
    IF (STAR_CROSS_COMPILE)
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist-windows)
    ELSE()
      SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/../dist)
    ENDIF()
  ENDIF()
ENDFOREACH (OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# External code included with starbound source, which core depends on
SET (STAR_EXTERN_INCLUDES ${PROJECT_SOURCE_DIR}/extern)
ADD_SUBDIRECTORY (extern)

# Core support code, not specific to starbound.
SET (STAR_CORE_INCLUDES ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/core/scripting)
ADD_SUBDIRECTORY (core)

# Less general purpose code than core that is available to both the game and
# application modules.
SET (STAR_BASE_INCLUDES ${PROJECT_SOURCE_DIR}/base ${PROJECT_SOURCE_DIR}/base/scripting)
ADD_SUBDIRECTORY (base)

# Platform APIs that are implemented by the application module
SET (STAR_PLATFORM_INCLUDES ${PROJECT_SOURCE_DIR}/platform)
ADD_SUBDIRECTORY (platform)

# Core game logic used by both server and client.
SET (STAR_GAME_INCLUDES
    ${PROJECT_SOURCE_DIR}/game
    ${PROJECT_SOURCE_DIR}/game/interfaces
    ${PROJECT_SOURCE_DIR}/game/items
    ${PROJECT_SOURCE_DIR}/game/objects
    ${PROJECT_SOURCE_DIR}/game/scripting
    ${PROJECT_SOURCE_DIR}/game/terrain
  )
ADD_SUBDIRECTORY (game)

# Googletest based tests # Don't need to compile these.
# ENABLE_TESTING()
# ADD_SUBDIRECTORY (test)

IF (NOT QT_GUIS_ONLY)

# Starbound stand-alone server.
ADD_SUBDIRECTORY (server)

# cmdline utilities
ADD_SUBDIRECTORY (utility)
ENDIF()

IF (STAR_BUILD_GUI)
  IF (NOT QT_GUIS_ONLY)
    # Handles creating windows, keyboard / mouse / joystick input, and the 2d
    # rendering model.
    SET (STAR_APPLICATION_INCLUDES ${PROJECT_SOURCE_DIR}/application)
    ADD_SUBDIRECTORY (application)

    # Rendering code not dependent on widget system
    SET (STAR_RENDERING_INCLUDES ${PROJECT_SOURCE_DIR}/rendering)
    ADD_SUBDIRECTORY (rendering)

    # Panes and Widgets
    SET (STAR_WINDOWING_INCLUDES ${PROJECT_SOURCE_DIR}/windowing)
    ADD_SUBDIRECTORY (windowing)

    # Client interface code
    SET (STAR_FRONTEND_INCLUDES ${PROJECT_SOURCE_DIR}/frontend)
    ADD_SUBDIRECTORY (frontend)

    # Starbound game / client
    ADD_SUBDIRECTORY (client)
  ENDIF()
  # Qt GUI tools
  IF (STAR_BUILD_QT_TOOLS)
    ADD_SUBDIRECTORY (json_tool)

    if (STAR_ENABLE_STEAM_INTEGRATION)
      ADD_SUBDIRECTORY (mod_uploader)
    ENDIF ()
  ENDIF ()
ENDIF ()
