add_library(tinyformat INTERFACE)

target_sources(tinyformat
        PUBLIC
        tinyformat.h
)

target_include_directories(tinyformat
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
