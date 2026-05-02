cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add ShaderTools to the required components
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        player_models
)

file(GLOB_RECURSE PLAYER_MODULE_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/player/*.qml"
)

# Intercommunication modules
add_library(player_models STATIC 
    src/models/lyricsmodel.cpp
    src/models/playlistmodel.cpp
    src/models/playerstate.cpp
)

target_link_libraries(player_models
    PRIVATE
        Qt6::Core
)

# The Player module
add_library(player_module STATIC)

qt_add_qml_module(player_module
    URI Player
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        ${PLAYER_MODULE_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player"
)

target_link_libraries(player_module
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)