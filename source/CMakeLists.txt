# External code included with starbound source, which core depends on
add_subdirectory(extern)

# Core support code, not specific to starbound.
add_subdirectory(core)

# Less general purpose code than core that is available to both the game and
# application modules.
add_subdirectory(base)

# Platform APIs that are implemented by the application module
add_subdirectory(platform)

# Core game logic used by both server and client.
add_subdirectory(game)

if(NOT QT_GUIS_ONLY)
    # Starbound stand-alone server.
    add_subdirectory(server)

    # cmdline utilities
    add_subdirectory(utility)
endif()

if(STAR_BUILD_GUI)
    if(NOT QT_GUIS_ONLY)
        # Handles creating windows, keyboard / mouse / joystick input, and the 2d
        # rendering model.
        add_subdirectory(application)

        # Rendering code not dependent on widget system
        add_subdirectory(rendering)

        # Panes and Widgets
        add_subdirectory(windowing)

        # Client interface code
        add_subdirectory(frontend)

        # Starbound game / client
        add_subdirectory(client)
    endif()

    # Qt GUI tools
    if(STAR_BUILD_QT_TOOLS)
        add_subdirectory(json_tool)

        if(STAR_ENABLE_STEAM_INTEGRATION)
            add_subdirectory(mod_uploader)
        endif()
    endif()
endif()
