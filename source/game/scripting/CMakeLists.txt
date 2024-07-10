add_library(star_game_scripting STATIC
        StarBehaviorLuaBindings.cpp
        StarBehaviorLuaBindings.hpp
        StarCelestialLuaBindings.cpp
        StarCelestialLuaBindings.hpp
        StarConfigLuaBindings.cpp
        StarConfigLuaBindings.hpp
        StarEntityLuaBindings.cpp
        StarEntityLuaBindings.hpp
        StarFireableItemLuaBindings.cpp
        StarFireableItemLuaBindings.hpp
        StarInputLuaBindings.cpp
        StarInputLuaBindings.hpp
        StarItemLuaBindings.cpp
        StarItemLuaBindings.hpp
        StarLuaActorMovementComponent.hpp
        StarLuaAnimationComponent.hpp
        StarLuaComponents.cpp
        StarLuaComponents.hpp
        StarLuaGameConverters.cpp
        StarLuaGameConverters.hpp
        StarLuaRoot.cpp
        StarLuaRoot.hpp
        StarMovementControllerLuaBindings.cpp
        StarMovementControllerLuaBindings.hpp
        StarNetworkedAnimatorLuaBindings.cpp
        StarNetworkedAnimatorLuaBindings.hpp
        StarPlayerLuaBindings.cpp
        StarPlayerLuaBindings.hpp
        StarRootLuaBindings.cpp
        StarRootLuaBindings.hpp
        StarScriptedAnimatorLuaBindings.cpp
        StarScriptedAnimatorLuaBindings.hpp
        StarStatusControllerLuaBindings.cpp
        StarStatusControllerLuaBindings.hpp
        StarUniverseServerLuaBindings.cpp
        StarUniverseServerLuaBindings.hpp
        StarUtilityLuaBindings.cpp
        StarUtilityLuaBindings.hpp
        StarWorldLuaBindings.cpp
        StarWorldLuaBindings.hpp
)

target_include_directories(star_game_scripting
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces
        ${CMAKE_CURRENT_SOURCE_DIR}/../objects
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_game_scripting
        PUBLIC
        Star::Base
        Star::Platform
)

target_precompile_headers(star_game_scripting REUSE_FROM star_core)

add_library(Star::GameScripting ALIAS star_game_scripting)

