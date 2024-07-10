if(STAR_ENABLE_STEAM_INTEGRATION)
    add_subdirectory(steam)
endif()

add_library(star_application STATIC
        StarApplication.cpp
        StarApplication.hpp
        StarApplicationController.hpp
        StarMainApplication.hpp
        StarMainApplication_sdl.cpp
        StarP2PNetworkingService_pc.cpp
        StarP2PNetworkingService_pc.hpp
        StarPlatformServices_pc.cpp
        StarPlatformServices_pc.hpp
        StarRenderer.cpp
        StarRenderer.hpp
        StarRenderer_opengl20.cpp
        StarRenderer_opengl20.hpp
        StarTextureAtlas.hpp
        $<$<TARGET_EXISTS:Star::ApplicationSteam>:$<TARGET_OBJECTS:Star::ApplicationSteam>>
)

target_include_directories(star_application
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_application
        PUBLIC
        Star::Platform
        Star::Core
        $<$<TARGET_EXISTS:Discord::SDK>:Discord::SDK>
        $<$<TARGET_EXISTS:Star::ApplicationSteam>:Star::ApplicationSteam>
        ${STAR_EXT_GUI_LIBS}
)

target_precompile_headers(star_application REUSE_FROM star_core)

add_library(Star::Application ALIAS star_application)
