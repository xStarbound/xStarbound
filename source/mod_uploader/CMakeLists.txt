set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

qt_add_executable(mod_uploader WIN32
        StarModUploader.cpp
        StarModUploader.hpp
        StarSPlainTextEdit.cpp
        StarSPlainTextEdit.hpp
        StarStringConversion.hpp
        main.cpp
)

target_link_libraries(mod_uploader
        PRIVATE
        Star::Base
        Steam::SDK
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
)

install(TARGETS mod_uploader
        RUNTIME_DEPENDENCY_SET STAR_RUNTIME_DEPS
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Tools
)

# Need to install Qt6 plug-ins for mod_uploader where it is defined.
qt_generate_deploy_app_script(TARGET mod_uploader
        OUTPUT_SCRIPT MOD_UPLOADER_DEPLOY_SCRIPT
        DEPLOY_TOOL_OPTIONS --no-compiler-runtime
        BIN_DIR ${STAR_INSTALL_BINDIR}
        LIB_DIR ${STAR_INSTALL_BINDIR}
        PLUGINS_DIR ${STAR_INSTALL_BINDIR}/plugins
)

install(SCRIPT ${MOD_UPLOADER_DEPLOY_SCRIPT})

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