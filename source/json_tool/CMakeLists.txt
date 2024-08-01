set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_executable(json_tool WIN32
        json_tool.cpp
        editor_gui.cpp
)

target_link_libraries(json_tool
        Star::Core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
)

install(TARGETS json_tool
        RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Tools
)

# Need to install Qt6 plug-ins for mod_uploader where it is defined.
qt_generate_deploy_app_script(TARGET json_tool
        OUTPUT_SCRIPT JSON_TOOL_DEPLOY_SCRIPT
        DEPLOY_TOOL_OPTIONS --no-compiler-runtime
        BIN_DIR ${STAR_INSTALL_BINDIR}
        LIB_DIR ${STAR_INSTALL_BINDIR}
        PLUGINS_DIR ${STAR_INSTALL_BINDIR}/plugins
)

install(SCRIPT ${JSON_TOOL_DEPLOY_SCRIPT})

if (STAR_SYSTEM_WINDOWS)
    # Install other linked, non-system libraries, as windeployqt misses a few DLLS (e.g. pcre2-16.dll)
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _regex_source_dir "${PROJECT_SOURCE_DIR}")
    string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _regex_binary_dir "${PROJECT_BINARY_DIR}")
    install(RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
            PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" # Virtual Windows runtime DLLs
            POST_INCLUDE_REGEXES "^${_regex_source_dir}/.+" "^${_regex_binary_dir}/.+"
            POST_EXCLUDE_REGEXES ".*"
            LIBRARY DESTINATION ${STAR_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${STAR_INSTALL_LIBDIR}
            FRAMEWORK DESTINATION ${STAR_INSTALL_LIBDIR}
            COMPONENT Libraries
    )
endif()