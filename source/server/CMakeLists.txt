
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
