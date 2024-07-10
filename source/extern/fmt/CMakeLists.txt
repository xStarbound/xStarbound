add_library(fmt STATIC
        fmt/core.h
        fmt/format.cc
        fmt/format.h
        fmt/format-inl.h
        fmt/ostream.h
        fmt/printf.h
        fmt/ranges.h
        fmt/std.h
)

target_include_directories(fmt
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
