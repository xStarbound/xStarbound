add_library(star_application_steam OBJECT
        StarDesktopService_pc_steam.cpp
        StarDesktopService_pc_steam.hpp
        StarStatisticsService_pc_steam.cpp
        StarStatisticsService_pc_steam.hpp
        StarUserGeneratedContentService_pc_steam.cpp
        StarUserGeneratedContentService_pc_steam.hpp
)

target_include_directories(star_application_steam
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(star_application_steam
        PRIVATE
        $<$<TARGET_EXISTS:Discord::SDK>:Discord::SDK>
        PUBLIC
        Star::Platform
        Star::Core
        Steam::SDK
)

add_library(Star::ApplicationSteam ALIAS star_application_steam)
