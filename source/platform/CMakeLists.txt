add_library(star_platform INTERFACE)

target_sources(star_platform
        PUBLIC
        StarP2PNetworkingService.hpp
        StarStatisticsService.hpp
        StarUserGeneratedContentService.hpp
)

target_include_directories(star_platform
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(Star::Platform ALIAS star_platform)
