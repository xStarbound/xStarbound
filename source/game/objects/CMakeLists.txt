add_library(star_game_objects STATIC
        StarContainerObject.cpp
        StarContainerObject.hpp
        StarFarmableObject.cpp
        StarFarmableObject.hpp
        StarLoungeableObject.cpp
        StarLoungeableObject.hpp
        StarPhysicsObject.cpp
        StarPhysicsObject.hpp
        StarTeleporterObject.cpp
        StarTeleporterObject.hpp
)
target_include_directories(star_game_objects
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces
        ${CMAKE_CURRENT_SOURCE_DIR}/../items
        ${CMAKE_CURRENT_SOURCE_DIR}/../scripting
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_game_objects
        PUBLIC
        Star::Base
        Star::Platform
)

target_precompile_headers(star_game_objects REUSE_FROM star_core)

add_library(Star::GameObjects ALIAS star_game_objects)
