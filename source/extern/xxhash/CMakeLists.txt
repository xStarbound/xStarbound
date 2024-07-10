add_library(xxhash STATIC
        xxh3.h
        xxh_x86dispatch.c
        xxh_x86dispatch.h
        xxhash.c
        xxhash.h
)

target_include_directories(xxhash
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
