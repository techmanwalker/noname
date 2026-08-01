cmake_minimum_required(VERSION 3.16)
project(noname LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 ShaderTools QmlIntegration LinguistTools)

find_package(SoundIo REQUIRED)

find_package(FFmpeg REQUIRED)

find_package(TagLib REQUIRED)

find_package(rapidfuzz REQUIRED)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_standard_project_setup(
    REQUIRES 6.10
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES es
)

### Base build file listing

file(GLOB_RECURSE PLAYER_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/fullscreen/*.qml"
)

file(GLOB_RECURSE BROWSER_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/browser/*.qml"
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

file(GLOB_RECURSE LISTINGS_QMLS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/qml/listings/*.qml"
)

file(GLOB QML_MEDIASEQUENCES_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    CONFIGURE_DEPENDS
    "src/presenters/sequences/*.hpp"
    "src/presenters/sequences/*.cpp"
)


### Inter-module communication

# Listings - turning a song to logical pixels - basic tooling to display media metadata

qt_add_qml_module(listings
    STATIC
    URI Player.Listings
    RESOURCE_PREFIX /player/listings
    QML_FILES
        ${LISTINGS_QMLS}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/Listings"
)

target_link_libraries(listings
    PUBLIC
        playereffects
        primitives
)

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
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        Qt6::QmlIntegration
        configuration
        diskio
        primitives
        rapidfuzz::rapidfuzz
        songfactory
        # must not depend on audioengine nor playerpresenter
)

target_include_directories(mediasequences
    PUBLIC
        src/presenters/formatters
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
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlayerPresenter"
)

target_link_libraries(playerpresenter
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Quick
        audioengine
        listings
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
qt_add_qml_module(fullscreenplayer
    STATIC
    URI Player.Fullscreen
    RESOURCE_PREFIX /player/fullscreen
    QML_FILES
        ${PLAYER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Fullscreen"
)

target_link_libraries(fullscreenplayer
    PUBLIC
        listings
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
qt_add_qml_module(browser
    STATIC
    URI Player.Browser
    RESOURCE_PREFIX /player/browser
    QML_FILES
        ${BROWSER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Browser"
)

target_link_libraries(browser
    PUBLIC
        listings
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
        songfactory

    PRIVATE
        Qt6::Core
        Qt6::Quick
        mediasequences
        primitives
        SoundIo::SoundIo
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swresample
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
        primitives
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swscale
        TagLib::tag
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

# Actual main executable - the Player

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${BASIC_LIBRARIES}
        audioengine
        mediasequences
        songfactory
)

qt_add_qml_module(${PROJECT_NAME}
    URI Player.App
    QML_FILES
        src/qml/Main.qml
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/App"
)

qt_add_translations(noname
    TS_FILE_BASE noname
    TS_OUTPUT_DIRECTORY src/translations
)

# Executable tests per module - filled with dummy data for easy development

if(BUILD_TESTS)

    qt_add_executable(mockupv2
        src/tests/mockupv2.cpp
    )

    target_link_libraries(mockupv2
        PUBLIC
            ${BASIC_LIBRARIES}
            fullscreenplayer
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

    qt_add_executable(backgroundgradinghelper
        src/tests/backgroundgradinghelper.cpp
    )

    target_link_libraries(backgroundgradinghelper
        PUBLIC
            ${BASIC_LIBRARIES}
            fullscreenplayer
            testqmls
    )

    qt_add_executable(startpagetest
        src/tests/startpage.cpp
    )

    target_link_libraries(startpagetest
        PUBLIC
            ${BASIC_LIBRARIES}
            audioengine
            browser
            configuration
            debugtools
            mediasequences
            fullscreenplayer
            playereffects
            primitives
            songfactory
            testqmls
    )

    target_include_directories(startpagetest
        PRIVATE
            ${PLAYER_INCLUDE_DIRECTORIES}
    )

    qt_add_executable (refreshlisttest
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
            testqmls
    )

    target_include_directories(refreshlisttest
        PRIVATE
            ${PLAYER_INCLUDE_DIRECTORIES}
    )

    qt_add_qml_module(testqmls
        STATIC
        URI Player.TestQmls
        QML_FILES
            src/tests/BackgroundGradingHelper.qml
            src/tests/MockupV2.qml
            src/tests/StartPageTest.qml
        OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/Player/TestQmls"
    )

endif()