# This file defines the basic install layout of Starbound for each operating system.

if(STAR_SYSTEM_WINDOWS)
    set(STAR_CONFIG_FILE_TEMPLATE "${PROJECT_SOURCE_DIR}/scripts/windows/xsbinit.config.in")
elseif(STAR_SYSTEM_MACOS)
    set(STAR_CONFIG_FILE_TEMPLATE "${PROJECT_SOURCE_DIR}/scripts/osx/xsbinit.config.in")

    # Required so xclient finds the Steam/Discord SDKs and additional frameworks in the bundle
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks/")
elseif(STAR_SYSTEM_LINUX OR STAR_SYSTEM_FAMILY_UNIX)
    set(STAR_CONFIG_FILE_TEMPLATE "${PROJECT_SOURCE_DIR}/scripts/linux/xsbinit.config.in")

    # Required so xclient finds the Steam/Discord SDKs without the need to set LD_LIBRARY_PATH,
    # which doesn't work anymore on some Linux distros due to security issues
    set(CMAKE_INSTALL_RPATH "\$ORIGIN")
endif()

# Create configuration files for installation & development
if(STAR_SYSTEM_MACOS)
    set(XSB_RES_BASE_DIR "../Resources")
else()
    set(XSB_RES_BASE_DIR "..")
endif()
configure_file(${STAR_CONFIG_FILE_TEMPLATE} ${PROJECT_BINARY_DIR}/xsbinit.config @ONLY)
set(XSB_RES_BASE_DIR "${STAR_GAME_RESOURCE_BASE_DIR}")
configure_file(${STAR_CONFIG_FILE_TEMPLATE} ${PROJECT_BINARY_DIR}/xsbinit.config.build @ONLY)

# Define install paths for use in targets
if(STAR_SYSTEM_WINDOWS)

    if(STAR_ARCHITECTURE STREQUAL "x86_64")
        set(STAR_INSTALL_BINDIR xsb-win64)
        set(STAR_INSTALL_LIBDIR xsb-win64)
    else()
        set(STAR_INSTALL_BINDIR xsb-win32)
        set(STAR_INSTALL_LIBDIR xsb-win32)
    endif()
    set(STAR_INSTALL_DATADIR .)

elseif(STAR_SYSTEM_MACOS)

    # MacOS is special, as we'll create a proper App bundle.
    set(STAR_INSTALL_BASEDIR "xSB Client.app/Contents/")
    set(STAR_INSTALL_BINDIR ${STAR_INSTALL_BASEDIR}/MacOS)
    set(STAR_INSTALL_LIBDIR ${STAR_INSTALL_BASEDIR}/Frameworks)
    set(STAR_INSTALL_DATADIR ${STAR_INSTALL_BASEDIR}/Resources)

    install(FILES macos/xClient.app/Contents/Resources/xsb.icns
            DESTINATION ${STAR_INSTALL_DATADIR}/
            COMPONENT Client
    )

    install(FILES macos/xClient.app/Contents/Info.plist
            DESTINATION ${STAR_INSTALL_BASEDIR}/
            COMPONENT Client
    )

elseif(STAR_SYSTEM_LINUX OR STAR_SYSTEM_FAMILY_UNIX)

    # Only use plain "linux", we don't distinguish as no one of sound mind runs a 32-bit Linux anymore.
    set(STAR_INSTALL_BINDIR linux)
    set(STAR_INSTALL_LIBDIR linux)
    set(STAR_INSTALL_DATADIR .)

endif()

# Used by Qt to install Qt libraries and plugins
set(CMAKE_INSTALL_BINDIR ${STAR_INSTALL_BINDIR})
set(CMAKE_INSTALL_LIBDIR ${STAR_INSTALL_LIBDIR})

# Assets and other folders

# Create "mods" dir as in the original Starbound game
file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/mods")
file(TOUCH "${PROJECT_SOURCE_DIR}/mods/mods_go_here")
file(WRITE "${PROJECT_BINARY_DIR}/assets/user/_metadata" "{\n  \"priority\" : 9999999999\n}")

install(FILES ${PROJECT_BINARY_DIR}/xsbinit.config
        DESTINATION ${STAR_INSTALL_BINDIR}/
        COMPONENT Assets
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/mods
        DESTINATION ${STAR_INSTALL_DATADIR}/
        COMPONENT Assets
)

install(FILES "${PROJECT_BINARY_DIR}/assets/user/_metadata"
        DESTINATION ${STAR_INSTALL_DATADIR}/assets/user/
        COMPONENT Assets
)

install(DIRECTORY doc
        DESTINATION ${STAR_INSTALL_DATADIR}/
        COMPONENT Assets
)

install(CODE "file(MAKE_DIRECTORY \${ENV}\${CMAKE_INSTALL_PREFIX}/${STAR_INSTALL_DATADIR}/storage/player \${ENV}\${CMAKE_INSTALL_PREFIX}/${STAR_INSTALL_DATADIR}/storage/universe)"
        COMPONENT Assets
)
