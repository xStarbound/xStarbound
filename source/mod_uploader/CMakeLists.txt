find_package(QT NAMES Qt6 Qt5 COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_executable(mod_uploader WIN32
        StarModUploader.cpp
        StarModUploader.hpp
        StarSPlainTextEdit.cpp
        StarSPlainTextEdit.hpp
        StarStringConversion.hpp
        main.cpp
)

target_link_libraries(mod_uploader
        Star::Base
        Steam::SDK
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
)
