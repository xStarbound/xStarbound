add_library(rpmalloc STATIC
    malloc.c
    rpmalloc.c
    rpmalloc.h
    rpnew.h
)

target_include_directories(rpmalloc
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)