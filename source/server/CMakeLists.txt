
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
