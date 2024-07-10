add_library(discord_game_sdk SHARED IMPORTED)

if(STAR_SYSTEM STREQUAL "windows")
    if(STAR_ARCHITECTURE STREQUAL "i386")
        set_target_properties(discord_game_sdk PROPERTIES
                IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/i386/discord_game_sdk.dll"
                IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/i386/discord_game_sdk.dll.lib"
        )
    elseif(STAR_ARCHITECTURE STREQUAL "x86_64")
        set_target_properties(discord_game_sdk PROPERTIES
                IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x86_64/discord_game_sdk.dll"
                IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x86_64/discord_game_sdk.dll.lib"
        )
    endif()
elseif(STAR_SYSTEM STREQUAL "macos")
    # This is a universal binary
    set_target_properties(discord_game_sdk PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/discord_game_sdk.dylib")
elseif(STAR_SYSTEM STREQUAL "linux")
    if(STAR_ARCHITECTURE STREQUAL "i386")
        message(FATAL_ERROR "Discord integration is not supported on 32-bit Linux!")
    elseif(STAR_ARCHITECTURE STREQUAL "x86_64")
        set_target_properties(discord_game_sdk PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/discord_game_sdk.so")
    endif()
elseif(STAR_SYSTEM STREQUAL "freebsd")
    message(FATAL_ERROR "Discord integration is not supported on FreeBSD!")
endif()

add_library(Discord::GameSDKLibrary ALIAS discord_game_sdk)

add_library(discord_sdk STATIC
        achievement_manager.cpp
        achievement_manager.h
        activity_manager.cpp
        activity_manager.h
        application_manager.cpp
        application_manager.h
        core.cpp
        core.h
        discord.h
        event.h
        ffi.h
        image_manager.cpp
        image_manager.h
        lobby_manager.cpp
        lobby_manager.h
        network_manager.cpp
        network_manager.h
        overlay_manager.cpp
        overlay_manager.h
        relationship_manager.cpp
        relationship_manager.h
        storage_manager.cpp
        storage_manager.h
        store_manager.cpp
        store_manager.h
        types.cpp
        types.h
        user_manager.cpp
        user_manager.h
        voice_manager.cpp
        voice_manager.h
)

target_include_directories(discord_sdk
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(discord_sdk
        INTERFACE
        Discord::GameSDKLibrary
)

add_library(Discord::SDK ALIAS discord_sdk)