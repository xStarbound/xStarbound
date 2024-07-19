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
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Tools
)

# Need to install Qt6 plug-ins for mod_uploader where it is defined.
qt_generate_deploy_app_script(TARGET json_tool
        OUTPUT_SCRIPT JSON_TOOL_DEPLOY_SCRIPT
        DEPLOY_TOOL_OPTIONS
        BIN_DIR ${STAR_INSTALL_BINDIR}
        LIB_DIR ${STAR_INSTALL_BINDIR}
        PLUGINS_DIR ${STAR_INSTALL_BINDIR}/plugins
)

install(SCRIPT ${JSON_TOOL_DEPLOY_SCRIPT})
