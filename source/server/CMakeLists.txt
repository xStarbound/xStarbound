
add_executable(xserver
        StarServerQueryThread.cpp
        StarServerQueryThread.hpp
        StarServerRconClient.cpp
        StarServerRconClient.hpp
        StarServerRconThread.cpp
        StarServerRconThread.hpp
        main.cpp
)

target_precompile_headers(xserver REUSE_FROM star_core)

target_link_libraries(xserver
        PRIVATE
        Star::Game
)

# Copy configuration file to the same dir as the executable for testing
add_custom_command(TARGET xserver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/xsbinit.config.build" "$<TARGET_FILE_DIR:xserver>/xsbinit.config"
)

install(TARGETS xserver
        RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Server
)

# Install any linked, non-system libraries alongside the game executable.
# This will include vcpkg-provided libraries, as they're residing in the binary dir.
string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _regex_source_dir "${PROJECT_SOURCE_DIR}")
string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _regex_binary_dir "${PROJECT_BINARY_DIR}")
install(RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
        PRE_EXCLUDE_REGEXES
            "libSDL2-2\.0\.so.*" # Only debug SDL2 library will be installed on Linux!
            "api-ms-" "ext-ms-" # Virtual Windows runtime DLLs
        POST_INCLUDE_REGEXES "^${_regex_source_dir}/.+" "^${_regex_binary_dir}/.+"
        POST_EXCLUDE_REGEXES ".*"
        LIBRARY DESTINATION ${STAR_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${STAR_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION ${STAR_INSTALL_LIBDIR}
        COMPONENT Libraries
)
