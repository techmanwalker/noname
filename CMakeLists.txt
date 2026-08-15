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

find_package(libjxl REQUIRED)

add_subdirectory(submodules/syrinc)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)

qt_standard_project_setup(
    REQUIRES 6.10
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES es
)

# Helper to reduce repetitive file(GLOB_RECURSE ...) boilerplate
function(module_sources out_var)
    set(files "")
    foreach(pattern IN LISTS ARGN)
        file(GLOB_RECURSE matched
            RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
            CONFIGURE_DEPENDS
            "${pattern}"
        )
        list(APPEND files ${matched})
    endforeach()
    set(${out_var} ${files} PARENT_SCOPE)
endfunction()


### Base build file listing

## QML files

module_sources(PLAYER_QMLS      "src/qml/fullscreen/*.qml")
module_sources(BROWSER_QMLS     "src/qml/browser/*.qml")
module_sources(UIPRIMITIVES_QMLS  "src/qml/primitives/*.qml")
module_sources(EFFECTS_QMLS     "src/qml/effects/*.qml")
module_sources(LISTINGS_QMLS    "src/qml/listings/*.qml")
module_sources(MAINWINDOW_QMLS  "src/qml/mainwindow/*.qml")

# otherwise Qt gets grumpy
set_source_files_properties("src/qml/primitives/MediaTypes.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)


## C++ sources
module_sources(MEDIATYPES_SOURCES       "src/presenters/mediatypes/*.cpp")
module_sources(MEDIASEQUENCES_SOURCES   "src/presenters/sequences/*.cpp")
module_sources(LYRICSMANIFEST_SOURCES   "src/presenters/context/lyrics/*.cpp")
module_sources(PLAYERPRESENTER_SOURCES  "src/presenters/context/playerpresenter/*.cpp")
module_sources(WINDOWGEOMETRY_SOURCES   "src/presenters/context/windowgeometry/*.cpp")
module_sources(LOCALLIBRARY_SOURCES     "src/presenters/context/locallibrary/*.cpp")
module_sources(AUDIOENGINE_SOURCES      "src/audioengine/*.cpp" "src/audioengine/*.hpp")
module_sources(COVERS_SOURCES           "src/covers/*.cpp")
module_sources(DEBUGTOOLS_SOURCES       "src/debug/*.cpp")
module_sources(MEMORY_SOURCES           "src/memory/*.cpp")
module_sources(DISKIO_SOURCES           "src/diskio/*.cpp")
module_sources(SONGFACTORY_SOURCES      "src/songfactory/*.cpp")

### C++ Modules

add_library(mediatypes STATIC
    ${MEDIATYPES_SOURCES}
)

target_link_libraries(mediatypes
    PUBLIC
        Qt6::Core
    
    PRIVATE
        memory
)

# Media sequences - playlists, queues and shorcuts representations

add_library(mediasequences
    STATIC
        ${MEDIASEQUENCES_SOURCES}
)

target_link_libraries(mediasequences
    PUBLIC
        mediatypes
        
    PRIVATE
        Qt6::Core

        rapidfuzz::rapidfuzz
)

target_include_directories(mediasequences
    PUBLIC
        src/presenters/sequences # to make qmltyperegistrar happy
)

target_include_directories(mediatypes
    PUBLIC
        src/presenters/formatters
        src/presenters/mediatypes
        src/presenters/rolecompiler
)

# The Playback controller
add_library(audioengine STATIC
    ${AUDIOENGINE_SOURCES}
)

target_link_libraries(audioengine
    PRIVATE
        mediatypes

        SoundIo::SoundIo
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swresample
)

target_include_directories(audioengine
    PUBLIC
        src/audioengine # enable including its own headers como Qt6:: libraries

)

# Covers - where your vision lives
add_library(covers STATIC
    ${COVERS_SOURCES}
)

target_link_libraries(covers
    PUBLIC
        Qt6::Quick
        mediatypes
        memory

    PRIVATE
        Qt6::Gui

        libjxl::libjxl
)

target_include_directories(covers
    PUBLIC
        src/covers
)

# The Song factory - enables extracting metadata, media and covers from songs themselves
add_library(songfactory STATIC
    ${SONGFACTORY_SOURCES}
)

target_link_libraries(songfactory
    PRIVATE
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swscale
        TagLib::tag

        mediatypes
)

target_include_directories(songfactory
    PUBLIC
        src/songfactory
)

# The debugging tools
add_library(debugtools STATIC
    ${DEBUGTOOLS_SOURCES}
)

target_link_libraries(debugtools
    PRIVATE
        mediatypes
)

target_include_directories(debugtools
    PUBLIC
        src/debug
)

# Configuration, caching and all data the player needs to remember
add_library(memory STATIC
    ${MEMORY_SOURCES}
)

target_link_libraries(memory
    PRIVATE
        Qt6::Core
)

target_include_directories(memory
    PUBLIC
        src/memory
)

# Basic misc disk operations
add_library(diskio STATIC
    ${DISKIO_SOURCES}
)

target_link_libraries(diskio
    PRIVATE
        Qt6::Core
)

target_include_directories(diskio
    PUBLIC
        src/diskio
)

### Inter-module communication

# Effects - any module depends on these to look nice

qt_add_qml_module(playereffects
    STATIC
    URI Player.Effects
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

# Primitives - basis of the style of the graphical view

qt_add_qml_module(uiprimitives
    STATIC
    URI Player.Primitives
    QML_FILES
        ${UIPRIMITIVES_QMLS}
        src/qml/primitives/Formatters.mjs
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Primitives"
)

target_link_libraries(uiprimitives
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Qml

        mediatypes
)

target_include_directories(uiprimitives
    PUBLIC
        src/presenters/basicdefinitions
)


# Listings - turning a song to logical pixels - basic tooling to display media metadata

qt_add_qml_module(listings
    STATIC
    URI Player.Listings
    QML_FILES
        ${LISTINGS_QMLS}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/Listings"
)

target_link_libraries(listings
    PUBLIC
        playereffects
        uiprimitives
)

# Playback presentation - everything that the C++ side exposes for your control and view

qt_add_qml_module(playerpresenter
    STATIC
    URI Player.PlayerPresenter
    SOURCES
        ${PLAYERPRESENTER_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlayerPresenter"
)

target_link_libraries(playerpresenter
    PRIVATE
        audioengine
        covers
        debugtools
        listings
        lyricsmanifest
        mediasequences
        songfactory
        memory
        uiprimitives

        # from syrinc
        lrc-core
        audio
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
    SOURCES
        ${LYRICSMANIFEST_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/LyricsManifest"
)

target_link_libraries(lyricsmanifest
    PRIVATE
        Qt6::Core
        mediatypes

        # from syrinc
        lrc-core
        audio
)

target_include_directories(lyricsmanifest
    PUBLIC
        src/presenters/context/lyrics
)


# The Player module
qt_add_qml_module(fullscreenplayer
    STATIC
    URI Player.Fullscreen
    QML_FILES
        ${PLAYER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Fullscreen"
)

target_link_libraries(fullscreenplayer
    PUBLIC
        playerpresenter
        uiprimitives

    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::ShaderTools
)

# The Start Page - your entry to the experience
qt_add_qml_module(browser
    STATIC
    URI Player.Browser
    QML_FILES
        ${BROWSER_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Browser"
)

target_link_libraries(browser
    PUBLIC
        listings
        locallibrary
        mediasequences
        uiprimitives

    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

# All of your collection of tracks noname has currently knowledge of
qt_add_qml_module(locallibrary
    STATIC
    URI Player.LocalLibrary
    SOURCES
        ${LOCALLIBRARY_SOURCES}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/LocalLibrary"
)

target_link_libraries (locallibrary
    PUBLIC
        mediasequences
        
    PRIVATE
        covers
        diskio
        mediatypes
        songfactory # local library interface, needs to load the songs metadata
        memory
)

target_include_directories(locallibrary
    PUBLIC
        src/presenters/context/locallibrary
)

### General excecutable linking definitions

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
        covers # every executable that features its own cover storage will feature this
        locallibrary # your entire collection
        mainwindow # window decorations and such
        playerpresenter

        songfactory # MUST be able to shut down its thread pool if something goes wrong
)

qt_add_qml_module(mainwindow
    STATIC
    URI Player.App
    SOURCES
        ${WINDOWGEOMETRY_SOURCES}
    QML_FILES
        ${MAINWINDOW_QMLS}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/App"
)

target_link_libraries(mainwindow
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Qml

        memory
)

target_include_directories(mainwindow
    PUBLIC
        src/presenters/context/windowgeometry
)

qt_add_translations(${PROJECT_NAME}
    TS_FILE_BASE ${PROJECT_NAME}
    TS_OUTPUT_DIRECTORY src/translations
)

# Executable tests per module - filled with dummy data for easy development

if(NONAME_BUILD_TESTS)
    # These tests are full fledged subsets of the main executable. You'll see basic deps repeated here.

    qt_add_executable(mockupv2
        src/tests/mockupv2.cpp
    )

    target_link_libraries(mockupv2
        PUBLIC
            audioengine
            covers
            fullscreenplayer
            mediasequences
            mediatypes
            memory
            songfactory
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
            fullscreenplayer
    )

    qt_add_executable(startpagetest
        src/tests/startpage.cpp
    )

    target_link_libraries(startpagetest
        PUBLIC
            audioengine
            browser
            covers
            debugtools
            fullscreenplayer
            songfactory
            locallibrary
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
            covers
            debugtools
            locallibrary
            mediatypes
            mediasequences
            memory
    )

    target_include_directories(refreshlisttest
        PRIVATE
            ${PLAYER_INCLUDE_DIRECTORIES}
    )

endif()