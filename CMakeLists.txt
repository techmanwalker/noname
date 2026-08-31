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

find_package(ICU REQUIRED COMPONENTS i18n uc)

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


## C++ sources
module_sources(MEDIATYPES_SOURCES       "src/presenters/mediatypes/*.cpp")
module_sources(MEDIASEQUENCES_SOURCES   "src/presenters/sequences/*.cpp")
module_sources(LYRICSMANIFEST_SOURCES   "src/presenters/context/lyrics/*.cpp")
module_sources(PLAYERPRESENTER_SOURCES  "src/presenters/context/playerpresenter/*.cpp")
module_sources(PLAYQUEUE_SOURCES        "src/presenters/context/playqueue/*.cpp")
module_sources(WINDOWGEOMETRY_SOURCES   "src/presenters/context/windowgeometry/*.cpp")
module_sources(LOCALLIBRARY_SOURCES     "src/presenters/context/locallibrary/*.cpp")
module_sources(AUDIOENGINE_SOURCES      "src/audioengine/*.cpp" "src/audioengine/*.hpp")
module_sources(COVERS_SOURCES           "src/covers/*.cpp")
module_sources(DEBUGTOOLS_SOURCES       "src/debug/*.cpp")
module_sources(CONFIGURATION_SOURCES    "src/configuration/*.cpp")
module_sources(DISKIO_SOURCES           "src/diskio/*.cpp")
module_sources(SONGFACTORY_SOURCES      "src/songfactory/*.cpp")

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




### C++ Modules

## --- Core domain and basic data structures

qt_add_qml_module(mediatypes
    STATIC
    URI Player.MediaTypes

    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/presenters/mediatypes/mediatypes_qml.hpp"
        ${MEDIATYPES_SOURCES}

    # comment on how linking works: anything that does not explicitly import the "Player.MediaTypes"
    # URI by itself won't link mediatypesplugin (QML backing target) and thus won't link anything
    # related to QML itself as per official documentation, so there's no dependency violation here.
    # Instantiated as a module to ensure type registration works on QML side.

    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/MediaTypes"
)

target_link_libraries(mediatypes
    PUBLIC
        Qt6::Core
)

target_include_directories(mediatypes
    PUBLIC
        src/presenters/formatters
        src/presenters/mediatypes
)

# Concatenation and natural cumulation and flow of basic mediatypes
# qml naturally deals with sequences to keep track, hence qt_add_qml_module
qt_add_qml_module(mediasequences
    STATIC
    URI Player.MediaSequences
    SOURCES
        ${MEDIASEQUENCES_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/MediaSequences"
)

target_link_libraries(mediasequences
    PUBLIC
        mediatypes
        
    PRIVATE
        Qt6::Core

        rolecompiler
)

target_include_directories(mediasequences
    PUBLIC
        src/presenters/sequences # to make qmltyperegistrar happy
)

# --- Pure I/O Tooling

# Basic misc disk operations - operates with text files and defining standard path schemes

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

# Configuration, caching and all settings the player needs to remember from boot time
add_library(configuration_li STATIC
    ${CONFIGURATION_SOURCES}
)

target_link_libraries(configuration_li
    PUBLIC
        configuration_in

    PRIVATE
        Qt6::Core

        diskio
)

target_include_directories(configuration_li
    PUBLIC
        src/configuration
)


add_library(configuration_in INTERFACE)
target_include_directories(configuration_in INTERFACE src/interfaces/configuration)
target_link_libraries(configuration_in INTERFACE Qt6::Core)



# The Song factory - enables extracting metadata, media and covers from songs themselves
# performs direct I/O with audio files

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


# --- Tooling - serialization and boilerplate reduction

# The debugging tools - mostly type serialization to console

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


# List model role compiler - easy declaration of qml roles with header only files

add_library(rolecompiler INTERFACE)

target_include_directories(rolecompiler
    INTERFACE 
        src/presenters/rolecompiler

)



## --- Sound I/O

add_library(audioengine_li STATIC
    ${AUDIOENGINE_SOURCES}
)

target_link_libraries(audioengine_li
    PUBLIC 
        audioengine_in # its interface

        mediatypes # legibly returns its track information

    PRIVATE
        SoundIo::SoundIo
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swresample
)

target_include_directories(audioengine_li
    PUBLIC
        src/audioengine # enable including its own headers como Qt6:: libraries
)

add_library(audioengine_in INTERFACE)
target_include_directories(audioengine_in INTERFACE src/interfaces/audioengine)
target_link_libraries(audioengine_in INTERFACE Qt6::Core)



## --- Local live storage modules - alive in memory ready to consume

# All of your collection of tracks noname has currently knowledge of
qt_add_qml_module(locallibrary_li
    STATIC
    URI Player.LocalLibrary
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/locallibrary/locallibraryproxy.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/searchresults/searchresultsproxy.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/shortcutslist/shortcutslistproxy.hpp"
        ${LOCALLIBRARY_SOURCES}
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/LocalLibrary"
)

target_link_libraries (locallibrary_li
    PUBLIC
        locallibrary_in
        searchresults_in
        shortcutslist_in
        
        mediasequences
        
    PRIVATE
        diskio
        configuration_in
        mediatypes
        rolecompiler
        songfactory # local library interface, needs to load the songs metadata

        rapidfuzz::rapidfuzz

        # nfkd and translit for search
        ICU::i18n
        ICU::uc
)

target_include_directories(locallibrary_li
    PUBLIC
        src/presenters/context/locallibrary
)

add_library(locallibrary_in INTERFACE)
target_include_directories(locallibrary_in INTERFACE src/interfaces/locallibrary)
target_link_libraries(locallibrary_in INTERFACE Qt6::Core mediatypes)

add_library(searchresults_in INTERFACE)
target_include_directories(searchresults_in INTERFACE src/interfaces/searchresults)
target_link_libraries(searchresults_in INTERFACE Qt6::Core mediatypes)

add_library(shortcutslist_in INTERFACE)
target_include_directories(shortcutslist_in INTERFACE src/interfaces/shortcutslist)
target_link_libraries(shortcutslist_in INTERFACE Qt6::Core mediatypes)

# Covers - where your vision lives, use the interface to register your cover on memory cache to display on any QML
add_library(covers STATIC
    ${COVERS_SOURCES}
)

target_link_libraries(covers
    PUBLIC
        Qt6::Quick

    PRIVATE
        Qt6::Gui

        diskio
        mediatypes
        configuration_in

        libjxl::libjxl

        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swscale
)

target_include_directories(covers
    PUBLIC
        src/covers
)


## --- Context presenters: everything the GUI can control and needs to watch on real time

# Lyrics manifest - access the lyrics from your playing song anywhere

qt_add_qml_module(lyricsmanifest_li
    STATIC
    URI Player.LyricsManifest
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/lyricsmanifest/lyricsmanifestproxy.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/lyricsmanifest/lyrictypes.hpp"
        ${LYRICSMANIFEST_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/LyricsManifest"
)

target_link_libraries(lyricsmanifest_li
    PUBLIC 
        lyricsmanifest_in

    PRIVATE
        Qt6::Core

        audioengine_in # track position, ask for current track if requested

        mediatypes
        rolecompiler

        # from syrinc
        lrc-core
        audio
)

target_include_directories(lyricsmanifest_li
    PUBLIC
        src/presenters/context/lyrics
)

add_library(lyricsmanifest_in INTERFACE)
target_include_directories(lyricsmanifest_in INTERFACE src/interfaces/lyricsmanifest)
target_link_libraries(lyricsmanifest_in INTERFACE Qt6::Core)


# What has played, what is playing and what is to be played

qt_add_qml_module(playqueue_li
    STATIC
    URI Player.PlayQueue
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/playqueue/playqueueproxy.hpp"
        ${PLAYQUEUE_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlayQueue"
)

target_link_libraries(playqueue_li
    PUBLIC
        playqueue_in
        
    PRIVATE
        audioengine_in # coordinates the progression and flow of the batch that's currently playing, controls what will play
        
        mediasequences
        songfactory # qml can drop songs to the play queue graphically, needs to read them
)

target_include_directories(playqueue_li
    PUBLIC
        src/presenters/context/playqueue
)

add_library(playqueue_in INTERFACE)
target_include_directories(playqueue_in INTERFACE src/interfaces/playqueue)
target_link_libraries(playqueue_in INTERFACE Qt6::Core)


# Playback presentation - playback control through simple button and indicator bindings

qt_add_qml_module(playerpresenter_li
    STATIC
    URI Player.PlayerPresenter
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/playerpresenter/playerpresenterproxy.hpp"
        ${PLAYERPRESENTER_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/PlayerPresenter"
)

target_link_libraries(playerpresenter_li
    PUBLIC
        playerpresenter_in

    PRIVATE
        audioengine_in # only has control over what is currently playing, hands switches and playlist/queue progression to playqueue
        playqueue_in

        configuration_in # load last volume from its save file
        mediatypes
)

target_include_directories(playerpresenter_li
    PUBLIC
        src/presenters/context/playerpresenter
)


add_library(playerpresenter_in INTERFACE)
target_include_directories(playerpresenter_in INTERFACE src/interfaces/playerpresenter)
target_link_libraries(playerpresenter_in INTERFACE Qt6::Core)



### The Graphical Interface

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
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/Player/Primitives"
)

target_link_libraries(uiprimitives
    PUBLIC
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
)

target_include_directories(uiprimitives
    PUBLIC
        src/presenters/basicdefinitions
)

# Media sequences - playlists, queues and shorcuts representations

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
        playerpresenter_in
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
        locallibrary_in
        uiprimitives

        mediatypes
        mediatypesplugin

    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

### General excecutable linking definitions

set(PLAYER_INCLUDE_DIRECTORIES
    src/presenters
)


### Executable building - composition root

# Actual main executable - the Player

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        audioengine_li # to safely tear it down
        locallibrary_li # your entire collection
        lyricsmanifest_li
        playerpresenter_li
        playqueue_li
        configuration_li

        covers # every executable that features its own cover storage will feature this

        mainwindow # window decorations and such

        songfactory # MUST be able to shut down its thread pool if something goes wrong

        debugtools # printing
)

qt_add_qml_module(mainwindow
    STATIC
    URI Player.App
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/configuration/windowgeometryproxy.hpp"
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

        configuration_in
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
    qt_add_executable(backgroundgradinghelper
        src/tests/backgroundgradinghelper.cpp
    )

    target_link_libraries(backgroundgradinghelper
        PUBLIC
            fullscreenplayer
    )
endif()

option(NONAME_ENABLE_QML_PROFILING "Enable QML debugging and profiling globally" OFF)
if(NONAME_ENABLE_QML_PROFILING)
    add_compile_definitions(QT_QML_DEBUG)
endif()