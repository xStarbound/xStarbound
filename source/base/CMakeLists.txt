add_library(star_base STATIC
        StarAnimatedPartSet.cpp
        StarAnimatedPartSet.hpp
        StarAssetSource.hpp
        StarAssets.cpp
        StarAssets.hpp
        StarBlocksAlongLine.hpp
        StarCellularLightArray.hpp
        StarCellularLighting.cpp
        StarCellularLighting.hpp
        StarCellularLiquid.hpp
        StarConfiguration.cpp
        StarConfiguration.hpp
        StarDirectoryAssetSource.cpp
        StarDirectoryAssetSource.hpp
        StarMemoryAssetSource.cpp
        StarMemoryAssetSource.hpp
        StarMixer.cpp
        StarMixer.hpp
        StarPackedAssetSource.cpp
        StarPackedAssetSource.hpp
        StarRootBase.cpp
        StarRootBase.hpp
        StarVersionOptionParser.cpp
        StarVersionOptionParser.hpp
        StarWorldGeometry.cpp
        StarWorldGeometry.hpp
        scripting/StarImageLuaBindings.cpp
        scripting/StarImageLuaBindings.hpp
)

target_include_directories(star_base
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripting
)

target_link_libraries(star_base
        PUBLIC
        Star::Core
)

target_precompile_headers(star_base REUSE_FROM star_core)

add_library(Star::Base ALIAS star_base)
