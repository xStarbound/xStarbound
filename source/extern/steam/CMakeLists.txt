add_library(steam_api_bindings SHARED IMPORTED)

if(STAR_SYSTEM STREQUAL "windows")
    if(STAR_ARCHITECTURE STREQUAL "i386")
        set_target_properties(steam_api_bindings PROPERTIES
                IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/steam_api.dll"
                IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/steam_api.lib"
        )
    elseif(STAR_ARCHITECTURE STREQUAL "x86_64")
        set_target_properties(steam_api_bindings PROPERTIES
                IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/steam_api64.dll"
                IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/steam_api64.lib"
        )
    endif()
elseif(STAR_SYSTEM STREQUAL "macos")
    # This is a universal binary
    set_target_properties(steam_api_bindings PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libsteam_api.dylib")
elseif(STAR_SYSTEM STREQUAL "linux")
    if(STAR_ARCHITECTURE STREQUAL "i386")
        set_target_properties(steam_api_bindings PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/i386/libsteam_api.so")
    elseif(STAR_ARCHITECTURE STREQUAL "x86_64")
        set_target_properties(steam_api_bindings PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/x86_64/libsteam_api.so")
    endif()
elseif(STAR_SYSTEM STREQUAL "freebsd")
    message(FATAL_ERROR "Steam integration is not supported on FreeBSD!")
endif()

add_library(Steam::APIBindings ALIAS steam_api_bindings)

add_library(steamworks_sdk INTERFACE)
set_target_properties(steamworks_sdk PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
        INTERFACE_LINK_LIBRARIES "Steam::APIBindings"
)

add_library(Steam::SDK ALIAS steamworks_sdk)