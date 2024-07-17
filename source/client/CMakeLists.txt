add_executable(xclient WIN32
        StarClientApplication.cpp
        StarClientApplication.hpp
        xclient.rc
)

target_precompile_headers(xclient REUSE_FROM star_core)

target_link_libraries(xclient
        PRIVATE
        Star::Frontend
)

if(STAR_COMPILER_GNU AND STAR_CROSS_COMPILE)
    # `-mwindows` stops cross-compiled xClient from opening up a console window when started.
    target_link_options(xclient BEFORE PUBLIC "-mwindows")
endif()
