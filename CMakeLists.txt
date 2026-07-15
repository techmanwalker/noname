cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 ShaderTools QmlIntegration Multimedia)

find_package(SoundIo REQUIRED)

find_package(FFmpeg REQUIRED)

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

# in Primitives
set_source_files_properties("src/qml/primitives/MediaTypes.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
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

file(GLOB QML_MEDIASEQUENCES_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/presenters/sequences/*.hpp"
    "src/presenters/sequences/*.cpp"
)


### Inter-module communication

# Media sequences - playlists, queues and shorcuts representations

qt_add_qml_module(mediasequences
    STATIC
    URI Player.MediaSequences
    RESOURCE_PREFIX /player/sequences
    SOURCES
        ${QML_MEDIASEQUENCES_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/MediaSequences"
)

target_link_libraries(mediasequences
    PUBLIC
        Qt6::Multimedia
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        Qt6::QmlIntegration
        configuration
        diskio
        primitives
        songfactory
        # must not depend on audioengine nor playerpresenter
)

target_include_directories(mediasequences
    PUBLIC
        src/presenters/sequences # to make qmltyperegistrar happy
)

# Playback presentation - everything that the C++ side exposes for your control and view

qt_add_qml_module(playerpresenter
    STATIC
    URI Player.PlayerPresenter
    RESOURCE_PREFIX /player/playerpresenter
    SOURCES
        src/presenters/context/playerpresenter/playerpresenter.hpp
        src/presenters/context/playerpresenter/playerpresenter.cpp
    DEPENDENCIES
        QtMultimedia
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlayerPresenter"
)

target_link_libraries(playerpresenter
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Quick
        audioengine
        mediasequences
        primitives
)

target_include_directories(playerpresenter
    PUBLIC
        src/presenters/context/playerpresenter

    PRIVATE
        src/presenters/sequences
)

# Lyrics manifest - access the lyrics from your playing song anywhere

qt_add_qml_module(lyricsmanifest
    STATIC
    URI Player.LyricsManifest
    RESOURCE_PREFIX /player/lyricsmanifest
    SOURCES
        src/presenters/context/lyrics/lyricsmanifest.hpp
        src/presenters/context/lyrics/lyricsmanifest.cpp
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
        src/presenters/context/lyrics
)

### QML Modules

# Primitives - the base of all modules

qt_add_qml_module(primitives
    STATIC
    URI Player.Primitives
    RESOURCE_PREFIX /primitives
    QML_FILES
        ${PRIMITIVES_QMLS}
        src/qml/primitives/Formatters.mjs
    SOURCES
        src/presenters/basicdefinitions/mediatypes.cpp
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Primitives"
)

target_include_directories(primitives
    PUBLIC
        src/presenters/basicdefinitions
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
        src/shaders/edge_fade.frag
        src/shaders/kawase_down.frag
        src/shaders/kawase_up.frag
        src/shaders/noise.frag
        src/shaders/noise_seed.frag
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
        playerpresenter
        lyricsmanifest

    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::ShaderTools
)

# The Start Page - your entry to the experience
qt_add_qml_module(startpage
    STATIC
    URI Player.StartPage
    RESOURCE_PREFIX /player/startpage
    QML_FILES
        ${STARTPAGE_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/StartPage"
)

target_link_libraries(startpage
    PUBLIC
        mediasequences
        primitives

    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)


### C++ Modules

# The Playback controller
add_library(audioengine STATIC
    src/audioengine/audiodecode.cpp
    src/audioengine/audioengine.cpp
    src/audioengine/loggingcategory.cpp
)

target_link_libraries(audioengine
    PUBLIC
        Qt6::Multimedia # every module linked to audioengine ends up needing this
        songfactory

    PRIVATE
        Qt6::Core
        Qt6::Quick
        mediasequences
        primitives
        SoundIo::SoundIo
        FFmpeg::FFmpeg
)

target_include_directories(audioengine
    PUBLIC
        src/audioengine # enable including its own headers como Qt6:: libraries

    PRIVATE
        src/presenters/sequences

)

# The Song factory - enables extracting metadata, media and covers from songs themselves
add_library(songfactory STATIC
    src/songfactory/coverprovider.cpp
    src/songfactory/coverproviderproxy.cpp
    src/songfactory/songfactory.cpp
)

target_link_libraries(songfactory
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Multimedia
        primitives
)

target_include_directories(songfactory
    PUBLIC
        src/songfactory
)

# The debugging tools
add_library(debugtools STATIC
    src/debug/dump.cpp
    src/debug/serialize.cpp
)

target_link_libraries(debugtools
    PRIVATE
        Qt6::Core
        mediasequences
        primitives
)

target_include_directories(debugtools
    PUBLIC
        src/debug
)

# The configuration
add_library(configuration STATIC
    src/configuration/configuration.cpp
)

target_link_libraries(configuration
    PRIVATE
        Qt6::Core
)

target_include_directories(configuration
    PUBLIC
        src/configuration
)

# Basic misc disk operations
add_library(diskio STATIC
    src/diskio/basicdiskio.cpp
    src/diskio/loggingcategory.cpp
)

target_link_libraries(diskio
    PRIVATE
        Qt6::Core
)

target_include_directories(diskio
    PUBLIC
        src/diskio
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
    src/presenters
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
        audioengine
        debugtools
        lyricsmanifest
        mediasequences
        configuration
        primitives
)

target_include_directories(mockupv2
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

qt_add_executable(backgroundgradinghelper EXCLUDE_FROM_ALL
    src/tests/backgroundgradinghelper.cpp
)

target_link_libraries(backgroundgradinghelper
    PUBLIC
        ${BASIC_LIBRARIES}
        player
)

target_include_directories(mockupv2
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

qt_add_executable(startpagetest EXCLUDE_FROM_ALL
    src/tests/startpage.cpp
)

target_link_libraries(startpagetest
    PUBLIC
        ${BASIC_LIBRARIES}
        audioengine
        configuration
        debugtools
        mediasequences
        player
        playereffects
        primitives
        songfactory
        startpage
)

target_include_directories(startpagetest
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

qt_add_executable (refreshlisttest EXCLUDE_FROM_ALL
    src/tests/refreshlist.cpp
)

target_link_libraries(refreshlisttest
    PUBLIC
        ${BASIC_LIBRARIES}
        configuration
        debugtools
        diskio
        mediasequences
        primitives
        songfactory
)

target_include_directories(refreshlisttest
    PRIVATE
        ${PLAYER_INCLUDE_DIRECTORIES}
)

# If BUILD_TESTS, add them to the default build target
if(BUILD_TESTS)
    set_target_properties(mockupv2 backgroundgradinghelper startpagetest refreshlisttest
        PROPERTIES EXCLUDE_FROM_ALL FALSE
    )
endif()