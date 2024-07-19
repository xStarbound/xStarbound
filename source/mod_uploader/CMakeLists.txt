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
        RUNTIME DESTINATION ${STAR_INSTALL_BINDIR}
        COMPONENT Tools
)

# Need to install Qt6 plug-ins for mod_uploader where it is defined.
qt_generate_deploy_app_script(TARGET mod_uploader
        OUTPUT_SCRIPT MOD_UPLOADER_DEPLOY_SCRIPT
        DEPLOY_TOOL_OPTIONS
        BIN_DIR ${STAR_INSTALL_BINDIR}
        LIB_DIR ${STAR_INSTALL_BINDIR}
        PLUGINS_DIR ${STAR_INSTALL_BINDIR}/plugins
)

install(SCRIPT ${MOD_UPLOADER_DEPLOY_SCRIPT})
