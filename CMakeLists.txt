cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 ShaderTools QmlIntegration Multimedia)

qt_policy(SET QTP0004 NEW)

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

file(GLOB_RECURSE EFFECTS_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/effects/*.qml"
)

file(GLOB QML_MODELS_SEQUENCES_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/models/sequences/*.hpp"
    "src/models/sequences/*.cpp"
    "src/models/abstractmediasequence.hpp"
    "src/models/abstractmediasequence.cpp"
    "src/models/defaultroles.hpp"
)


### Inter-module communication

# Media sequences - playlists, queues and shorcuts representations

qt_add_qml_module(mediasequences
    STATIC
    URI Player.MediaSequences
    RESOURCE_PREFIX /player/sequences
    SOURCES
        ${QML_MODELS_SEQUENCES_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/MediaSequences"
)

target_link_libraries(mediasequences
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
)

target_include_directories(mediasequences
    PUBLIC
        src/models
    PRIVATE
        src/models/sequences # to make qmltyperegistrar happy
)

# Playback presentation - everything that the C++ side exposes for your control and view

qt_add_qml_module(playbackpresentation
    STATIC
    URI Player.PlaybackPresentation
    RESOURCE_PREFIX /player/playbackpresentation
    SOURCES
        src/models/playbackpresentation/playbackpresentation.hpp
        src/models/playbackpresentation/playbackpresentation.cpp
    DEPENDENCIES
        QtMultimedia
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlaybackPresentation"
)

target_link_libraries(playbackpresentation
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Quick
        playbackcontroller
)

target_include_directories(playbackpresentation
    PUBLIC
        src/models
        src/playbackcontroller
    PRIVATE
        src/models/playbackpresentation
)

# Lyrics manifest - access the lyrics from your playing song anywhere

qt_add_qml_module(lyricsmanifest
    STATIC
    URI Player.LyricsManifest
    RESOURCE_PREFIX /player/lyricsmanifest
    SOURCES
        src/models/lyrics/lyricsmanifest.hpp
        src/models/lyrics/lyricsmanifest.cpp
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/LyricsManifest"
)

target_link_libraries(lyricsmanifest
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
)

target_include_directories(lyricsmanifest
    PUBLIC
        src/models
    PRIVATE
        src/models/lyrics
)

### QML Modules

# Primitives - the base of all modules

qt_add_qml_module(primitives
    STATIC
    URI Player.Primitives
    RESOURCE_PREFIX /primitives
    QML_FILES
        ${PRIMITIVES_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Primitives"
)

# Effects - any module depends on these to look nice

qt_add_qml_module(playereffects
    STATIC
    URI Player.Effects
    RESOURCE_PREFIX /player/effects
    QML_FILES
        ${EFFECTS_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Effects"
)

qt_add_shaders(playereffects "_shaders"
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
qt_add_qml_module(player
    STATIC
    URI Player
    RESOURCE_PREFIX /player
    QML_FILES
        ${PLAYER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player"
)

target_link_libraries(player
    PUBLIC
        mediasequences
        playbackpresentation
        lyricsmanifest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::ShaderTools
)

# The Start page module

qt_add_qml_module(startpage
    STATIC
    URI StartPage
    RESOURCE_PREFIX /startpage
    QML_FILES
        ${STARTPAGE_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/StartPage"
)

target_link_libraries(startpage
    PRIVATE
        mediasequences
)

target_include_directories(startpage
    PRIVATE
        src/models
)


### C++ Modules

# The Playback controller
add_library(playbackcontroller STATIC
    src/playbackcontroller/playbackcontroller.cpp
    src/playbackcontroller/coverprovider.cpp
)

target_link_libraries(playbackcontroller
    PUBLIC
        Qt6::Multimedia # every module linked to playbackcontroller ends up needing this
    PRIVATE
        Qt6::Core
        Qt6::Quick
)

target_include_directories(playbackcontroller
    PRIVATE
        src/models
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
    PUBLIC
        ${BASIC_LIBRARIES}
        player
        playereffects
        primitives
        playbackcontroller
)

target_include_directories(mockupv2
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

qt_add_executable(startpagetest EXCLUDE_FROM_ALL
    src/tests/startpagetest.cpp
)

target_link_libraries(startpagetest
    PRIVATE
        ${BASIC_LIBRARIES}
        startpage
        player
)

target_include_directories(startpagetest
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

# If BUILD_TESTS, add them to the default build target
if(BUILD_TESTS)
    set_target_properties(mockupv2 startpagetest
        PROPERTIES EXCLUDE_FROM_ALL FALSE
    )
endif()