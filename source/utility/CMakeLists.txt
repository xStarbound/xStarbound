add_executable(asset_packer
        asset_packer.cpp
)
target_link_libraries(asset_packer
        Star::Base
)

add_executable(asset_unpacker
        asset_unpacker.cpp
)
target_link_libraries(asset_unpacker
        Star::Base
)

add_executable(dump_versioned_json
        dump_versioned_json.cpp
)
target_link_libraries(dump_versioned_json
        Star::Game
)

add_executable(game_repl
        game_repl.cpp
)
target_link_libraries(game_repl
        Star::Game
)

add_executable(make_versioned_json
        make_versioned_json.cpp
)
target_link_libraries(make_versioned_json
        Star::Game
)

add_executable(planet_mapgen
        planet_mapgen.cpp
)
target_link_libraries(planet_mapgen
        Star::Game
)

add_executable(render_terrain_selector
        render_terrain_selector.cpp
)
target_link_libraries(render_terrain_selector
        Star::Game
)

add_executable(update_tilesets
        update_tilesets.cpp
        tileset_updater.cpp
)
target_link_libraries(update_tilesets
        Star::Game
)

add_executable(fix_embedded_tilesets
        fix_embedded_tilesets.cpp
)

target_link_libraries(fix_embedded_tilesets
        Star::Game
)

add_executable(world_benchmark
        world_benchmark.cpp
)

target_link_libraries(world_benchmark
        Star::Game
)

add_executable(generation_benchmark
        generation_benchmark.cpp
)
target_link_libraries(generation_benchmark
        Star::Game
)

add_executable(dungeon_generation_benchmark
        dungeon_generation_benchmark.cpp
)
target_link_libraries(dungeon_generation_benchmark
        Star::Game
)

# xSB-2 v2.5 breaks `word_count`. Might as well get rid of it and `map_grep`.
# add_executable(map_grep map_grep.cpp)
# target_link_libraries (map_grep Star::Game)

# add_executable (word_count word_count.cpp)
# target_link_libraries (word_count Star::Game)

# Kae's `btree_repacker`.
add_executable(btree_repacker
        btree_repacker.cpp
)

target_link_libraries(btree_repacker
        Star::Base
)

install(TARGETS
        asset_packer
        asset_unpacker
        dump_versioned_json
        make_versioned_json
        planet_mapgen
        RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Tools
        OPTIONAL
)

if(STAR_INSTALL_EXTRA_TOOLS)
    install(TARGETS
            btree_repacker
            dungeon_generation_benchmark
            fix_embedded_tilesets
            game_repl
            generation_benchmark
            render_terrain_selector
            update_tilesets
            world_benchmark
            RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
            RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
            COMPONENT Tools
            OPTIONAL
    )
endif()
