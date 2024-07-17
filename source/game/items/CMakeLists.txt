add_library(star_game_items STATIC
        StarActiveItem.cpp
        StarActiveItem.hpp
        StarArmors.cpp
        StarArmors.hpp
        StarAugmentItem.cpp
        StarAugmentItem.hpp
        StarBlueprintItem.cpp
        StarBlueprintItem.hpp
        StarCodexItem.cpp
        StarCodexItem.hpp
        StarConsumableItem.cpp
        StarConsumableItem.hpp
        StarCurrency.cpp
        StarCurrency.hpp
        StarInspectionTool.cpp
        StarInspectionTool.hpp
        StarInstrumentItem.cpp
        StarInstrumentItem.hpp
        StarLiquidItem.cpp
        StarLiquidItem.hpp
        StarMaterialItem.cpp
        StarMaterialItem.hpp
        StarObjectItem.cpp
        StarObjectItem.hpp
        StarThrownItem.cpp
        StarThrownItem.hpp
        StarTools.cpp
        StarTools.hpp
        StarUnlockItem.cpp
        StarUnlockItem.hpp
)

target_include_directories(star_game_items
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces
        ${CMAKE_CURRENT_SOURCE_DIR}/../scripting
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_game_items
        PUBLIC
        Star::Base
        Star::Platform
)

target_precompile_headers(star_game_items REUSE_FROM star_core)

add_library(Star::GameItems ALIAS star_game_items)
