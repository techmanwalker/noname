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
module_sources(PRIMITIVES_QMLS  "src/qml/primitives/*.qml")
module_sources(EFFECTS_QMLS     "src/qml/effects/*.qml")
module_sources(LISTINGS_QMLS    "src/qml/listings/*.qml")

# otherwise Qt gets grumpy
set_source_files_properties("src/qml/primitives/MediaTypes.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)


## C++ sources
module_sources(MEDIASEQUENCES_SOURCES   "src/presenters/sequences/*.cpp")
module_sources(PLAYERPRESENTER_SOURCES  "src/presenters/context/playerpresenter/*.cpp")
module_sources(LYRICSMANIFEST_SOURCES   "src/presenters/context/lyrics/*.cpp")
module_sources(AUDIOENGINE_SOURCES      "src/audioengine/*.cpp")
module_sources(COVERS_SOURCES           "src/covers/*.cpp")
module_sources(DEBUGTOOLS_SOURCES       "src/debug/*.cpp")
module_sources(MEMORY_SOURCES           "src/memory/*.cpp")
module_sources(DISKIO_SOURCES           "src/diskio/*.cpp")
module_sources(SONGFACTORY_SOURCES      "src/songfactory/*.cpp")

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

# Primitives - the base of all modules

qt_add_qml_module(primitives
    STATIC
    URI Player.Primitives
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
        primitives
)

# Media sequences - playlists, queues and shorcuts representations

qt_add_qml_module(mediasequences
    STATIC
    URI Player.MediaSequences
    SOURCES
        ${MEDIASEQUENCES_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Player/MediaSequences"
)

target_link_libraries(mediasequences
    PUBLIC
        covers
        songfactory
        
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Qml
        Qt6::QmlIntegration
        diskio
        memory
        primitives
        rapidfuzz::rapidfuzz
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
    SOURCES
        ${PLAYERPRESENTER_SOURCES}
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
    SOURCES
        ${LYRICSMANIFEST_SOURCES}
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
    ${AUDIOENGINE_SOURCES}
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

# Covers - where your vision lives
add_library(covers STATIC
    ${COVERS_SOURCES}
)

target_link_libraries(covers
    PUBLIC
        memory

    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick

        libjxl::libjxl
)

target_include_directories(covers
    PUBLIC
        src/covers
)

# The Song factory - enables extracting metadata, media and covers from songs themselves
add_library(songfactory STATIC
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
        covers # songfactory still extracts covers on its own, which must change
        memory
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
        Qt6::Core
        mediasequences
        primitives
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
        browser
        fullscreenplayer
        mediasequences
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
            audioengine
            fullscreenplayer
            mediasequences
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
    )

    qt_add_executable(startpagetest
        src/tests/startpage.cpp
    )

    target_link_libraries(startpagetest
        PUBLIC
            ${BASIC_LIBRARIES}
            audioengine
            browser
            debugtools
            fullscreenplayer
            mediasequences
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
            covers
            debugtools
            diskio
            mediasequences
            memory
            primitives
            songfactory
    )

    target_include_directories(refreshlisttest
        PRIVATE
            ${PLAYER_INCLUDE_DIRECTORIES}
    )

endif()