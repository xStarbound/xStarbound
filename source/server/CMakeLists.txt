INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
    ${STAR_PLATFORM_INCLUDES}
    ${STAR_GAME_INCLUDES}
  )

SET (star_server_HEADERS
    StarServerQueryThread.hpp
    StarServerRconClient.hpp
    StarServerRconThread.hpp
  )

SET (star_server_SOURCES
    StarServerQueryThread.cpp
    StarServerRconClient.cpp
    StarServerRconThread.cpp
    main.cpp
  )

ADD_EXECUTABLE (xserver
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  ${star_server_HEADERS} ${star_server_SOURCES})
TARGET_PRECOMPILE_HEADERS (xserver REUSE_FROM star_core)
TARGET_LINK_LIBRARIES (xserver ${STAR_EXT_LIBS})