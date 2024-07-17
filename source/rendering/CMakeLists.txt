add_library(star_rendering STATIC
        StarAnchorTypes.cpp
        StarAnchorTypes.hpp
        StarAssetTextureGroup.cpp
        StarAssetTextureGroup.hpp
        StarDrawablePainter.cpp
        StarDrawablePainter.hpp
        StarEnvironmentPainter.cpp
        StarEnvironmentPainter.hpp
        StarFontTextureGroup.cpp
        StarFontTextureGroup.hpp
        StarTextPainter.cpp
        StarTextPainter.hpp
        StarTilePainter.cpp
        StarTilePainter.hpp
        StarWorldCamera.cpp
        StarWorldCamera.hpp
        StarWorldPainter.cpp
        StarWorldPainter.hpp
)

target_include_directories(star_rendering
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_rendering
        PUBLIC
        Star::Application
        Star::Game
        Star::Base
)

target_precompile_headers(star_rendering REUSE_FROM star_core)

add_library(Star::Rendering ALIAS star_rendering)

