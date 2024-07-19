find_package(GTest REQUIRED)

add_executable(core_tests
        algorithm_test.cpp
        block_allocator_test.cpp
        blocks_along_line_test.cpp
        btree_database_test.cpp
        btree_test.cpp
        byte_array_test.cpp
        clock_test.cpp
        color_test.cpp
        container_test.cpp
        encode_test.cpp
        file_test.cpp
        hash_test.cpp
        host_address_test.cpp
        ref_ptr_test.cpp
        json_test.cpp
        flat_hash_test.cpp
        formatted_json_test.cpp
        line_test.cpp
        lua_test.cpp
        lua_json_test.cpp
        math_test.cpp
        multi_table_test.cpp
        net_states_test.cpp
        ordered_map_test.cpp
        ordered_set_test.cpp
        periodic_test.cpp
        poly_test.cpp
        random_test.cpp
        rect_test.cpp
        serialization_test.cpp
        static_vector_test.cpp
        small_vector_test.cpp
        sha_test.cpp
        shell_parse.cpp
        string_test.cpp
        strong_typedef_test.cpp
        thread_test.cpp
        worker_pool_test.cpp
        variant_test.cpp
        vlq_test.cpp
)
target_link_libraries(core_tests
        Star::Base
        GTest::gtest_main
        GTest::gtest
)
add_test(NAME core_tests WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/core_tests)

add_executable(game_tests
        game_tests_main.cpp

        StarTestUniverse.cpp
        assets_test.cpp
        function_test.cpp
        item_test.cpp
        root_test.cpp
        server_test.cpp
        spawn_test.cpp
        stat_test.cpp
        tile_array_test.cpp
        world_geometry_test.cpp
        universe_connection_test.cpp
)

target_link_libraries(game_tests
        Star::Game
        GTest::gtest
)

if(EXISTS "${PROJECT_SOURCE_DIR}/assets/packed.pak" AND (EXISTS "${PROJECT_SOURCE_DIR}/xsb-assets/packed.pak" OR TARGET package_xsb_assets))
    add_test(NAME game_tests WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/game_tests)

    # Make sure the xSB assets are packaged when building the test, e.g. if the dev does not build the ALL target.
    if(TARGET package_xsb_assets)
        add_dependencies(game_tests package_xsb_assets)
    endif()
else()
    message(AUTHOR_WARNING "Skipped adding game_tests to CTest configuration.\n"
            "Make sure the packed asset files are present in both assets/ and xsb-assets/ in the main project directory!\n"
            "You can still run the game tests manually though, or reconfigure the project after adding the paks.")
endif()
