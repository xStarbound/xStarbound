add_library(star_game_interfaces STATIC
        StarActivatableItem.hpp
        StarAggressiveEntity.hpp
        StarAnchorableEntity.cpp
        StarAnchorableEntity.hpp
        StarBeamItem.cpp
        StarBeamItem.hpp
        StarChattyEntity.hpp
        StarContainerEntity.cpp
        StarContainerEntity.hpp
        StarDamageBarEntity.cpp
        StarDamageBarEntity.hpp
        StarDurabilityItem.hpp
        StarEffectSourceItem.hpp
        StarEmoteEntity.hpp
        StarEntity.cpp
        StarEntity.hpp
        StarFireableItem.cpp
        StarFireableItem.hpp
        StarInspectableEntity.hpp
        StarInteractiveEntity.cpp
        StarInteractiveEntity.hpp
        StarLoungingEntities.cpp
        StarLoungingEntities.hpp
        StarNametagEntity.hpp
        StarNonRotatedDrawablesItem.hpp
        StarPhysicsEntity.cpp
        StarPhysicsEntity.hpp
        StarPointableItem.cpp
        StarPointableItem.hpp
        StarPortraitEntity.hpp
        StarPreviewableItem.hpp
        StarPreviewTileTool.hpp
        StarRenderableItem.hpp
        StarScriptedEntity.hpp
        StarStatusEffectEntity.hpp
        StarStatusEffectItem.hpp
        StarSwingableItem.cpp
        StarSwingableItem.hpp
        StarTileEntity.cpp
        StarTileEntity.hpp
        StarToolUserEntity.hpp
        StarToolUserItem.cpp
        StarToolUserItem.hpp
        StarWarpTargetEntity.hpp
        StarWireEntity.hpp
        StarWorld.cpp
        StarWorld.hpp
)

target_include_directories(star_game_interfaces
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../scripting
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_game_interfaces
        PUBLIC
        Star::Base
        Star::Platform
)

target_precompile_headers(star_game_interfaces REUSE_FROM star_core)

add_library(Star::GameInterfaces ALIAS star_game_interfaces)

