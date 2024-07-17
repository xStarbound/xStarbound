add_library(star_game_terrain STATIC
        StarCacheSelector.cpp
        StarCacheSelector.hpp
        StarConstantSelector.cpp
        StarConstantSelector.hpp
        StarDisplacementSelector.cpp
        StarDisplacementSelector.hpp
        StarFlatSurfaceSelector.cpp
        StarFlatSurfaceSelector.hpp
        StarIslandSurfaceSelector.cpp
        StarIslandSurfaceSelector.hpp
        StarKarstCave.cpp
        StarKarstCave.hpp
        StarMaxSelector.cpp
        StarMaxSelector.hpp
        StarMinMaxSelector.cpp
        StarMinMaxSelector.hpp
        StarMixSelector.cpp
        StarMixSelector.hpp
        StarPerlinSelector.cpp
        StarPerlinSelector.hpp
        StarRidgeBlocksSelector.cpp
        StarRidgeBlocksSelector.hpp
        StarRotateSelector.cpp
        StarRotateSelector.hpp
        StarWormCave.cpp
        StarWormCave.hpp
)

target_include_directories(star_game_terrain
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_game_terrain
        PUBLIC
        Star::Base
        Star::Platform
)

target_precompile_headers(star_game_terrain REUSE_FROM star_core)

add_library(Star::GameTerrain ALIAS star_game_terrain)
