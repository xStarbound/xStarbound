add_executable(xclient WIN32
        StarClientApplication.cpp
        StarClientApplication.hpp
        xclient.rc
)

target_precompile_headers(xclient REUSE_FROM star_core)

target_link_libraries(xclient
        PRIVATE
        Star::Frontend
)

if(STAR_COMPILER_GNU AND STAR_CROSS_COMPILE)
    # `-mwindows` stops cross-compiled xClient from opening up a console window when started.
    target_link_options(xclient BEFORE PUBLIC "-mwindows")
endif()

# Copy configuration file to the same dir as the executable for testing
add_custom_command(TARGET xclient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/xsbinit.config.build" "$<TARGET_FILE_DIR:xclient>/xsbinit.config"
)

# Also copy Steam/Discord SDK libraries over to the build dir on Windows. UNIX has RPATH for this.
if (STAR_SYSTEM_WINDOWS)
    if(STAR_ENABLE_DISCORD_INTEGRATION)
        add_custom_command(TARGET xclient POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${STAR_DISCORD_SDK_LIBRARY_FILE}" "$<TARGET_FILE_DIR:xclient>"
        )
    endif()
    if(STAR_ENABLE_STEAM_INTEGRATION)
        add_custom_command(TARGET xclient POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${STAR_STEAM_SDK_LIBRARY_FILE}" "$<TARGET_FILE_DIR:xclient>"
        )
    endif()
endif()
