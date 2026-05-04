cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add ShaderTools to the required components
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 ShaderTools)

set(PLAYER_LINKED_LIBRARIES
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    player_models
    player_module
)

set(PLAYER_INCLUDE_DIRECTORIES
    src/models
)

qt_add_executable(${PROJECT_NAME}
    # currently empty
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PLAYER_LINKED_LIBRARIES}
)

qt_add_executable(mockupv2
    src/tests/mockupv2.cpp
)

target_link_libraries(mockupv2
    PRIVATE
        ${PLAYER_LINKED_LIBRARIES}
)

target_include_directories(mockupv2
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)


file(GLOB_RECURSE PLAYER_MODULE_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/*.qml"
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

qt_add_shaders(player_module "player_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX "/shaders"
    BASE "src/shaders"
    FILES
        src/shaders/background_overlay.frag
        src/shaders/kawase_down.frag
        src/shaders/kawase_up.frag
)

target_link_libraries(player_module
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::ShaderTools
)