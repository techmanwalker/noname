cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 ShaderTools)


### Base build file listing

file(GLOB_RECURSE PLAYER_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/player/*.qml"
)

file(GLOB_RECURSE STARTPAGE_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/startpage/*.qml"
)

file(GLOB_RECURSE PRIMITIVES_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/primitives/*.qml"
)

file(GLOB_RECURSE SHADERS_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/shaders/*.qml"
)


### Inter-module communication

add_library(models STATIC 
    src/models/lyricsmodel.cpp
    src/models/abstractmodel.cpp
    src/models/playlistmodel.cpp
    src/models/playerstate.cpp
)

target_link_libraries(models
    PRIVATE
        Qt6::Core
)


### Modules

# Primitives - the base of all modules

add_library(primitives STATIC)

qt_add_qml_module(primitives
    URI Primitives
    VERSION 1.0
    RESOURCE_PREFIX /primitives
    QML_FILES
        ${PRIMITIVES_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Primitives"
)

# Shaders - any module depends on these to look nice

add_library(shaders STATIC)

qt_add_qml_module(shaders
    URI Shaders
    VERSION 1.0
    RESOURCE_PREFIX /shaders
    QML_FILES
        ${SHADERS_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Shaders"
)

qt_add_shaders(shaders "_shaders"
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

# The Player module
add_library(player STATIC)

qt_add_qml_module(player
    URI Player
    VERSION 1.0
    RESOURCE_PREFIX /player
    QML_FILES
        ${PLAYER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player"
)

target_link_libraries(player
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::ShaderTools
)


# The Start page module

add_library(startpage STATIC)

qt_add_qml_module(startpage
    URI StartPage
    VERSION 1.0
    RESOURCE_PREFIX /startpage
    QML_FILES
        ${STARTPAGE_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/StartPage"
)


### General excecutable linking definitions

set(BASIC_LIBRARIES
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    primitives
)

set(PLAYER_INCLUDE_DIRECTORIES
    src/models
)


### Executable building

# Actual main executable - currently empty

qt_add_executable(${PROJECT_NAME}
    # currently empty
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${BASIC_LIBRARIES}
)


# Executable tests per module - filled with dummy data for easy development

qt_add_executable(mockupv2 EXCLUDE_FROM_ALL
    src/tests/mockupv2.cpp
)

target_link_libraries(mockupv2
    PRIVATE
        ${BASIC_LIBRARIES}
        models
        player
        shaders
        primitives
)

target_include_directories(mockupv2
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

qt_add_executable(mainpagetest EXCLUDE_FROM_ALL
    src/tests/mainpage.cpp
)

target_link_libraries(mainpagetest
    PRIVATE
        ${BASIC_LIBRARIES}
        startpage
)

# If BUILD_TESTS, add them to the default build target
if(BUILD_TESTS)
    set_target_properties(mockupv2 mainpagetest
        PROPERTIES EXCLUDE_FROM_ALL FALSE
    )
endif()