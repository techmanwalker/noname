# Module to find FFmpeg libraries.
# Provides individual imported targets so consumers only pull what they
# actually use: FFmpeg::avutil, FFmpeg::swresample, FFmpeg::swscale,
# FFmpeg::avcodec, FFmpeg::avformat. Each expresses its real dependency on
# the others, so e.g. linking only FFmpeg::avformat still transitively
# picks up avcodec + avutil, but never swscale/swresample if you don't
# also ask for those.

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    DOC "Path to FFmpeg headers"
)

find_library(AVUTIL_LIBRARY     NAMES avutil)
find_library(SWRESAMPLE_LIBRARY NAMES swresample)
find_library(SWSCALE_LIBRARY    NAMES swscale)
find_library(AVCODEC_LIBRARY    NAMES avcodec)
find_library(AVFORMAT_LIBRARY   NAMES avformat)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_INCLUDE_DIR AVUTIL_LIBRARY SWRESAMPLE_LIBRARY SWSCALE_LIBRARY AVCODEC_LIBRARY AVFORMAT_LIBRARY
)

if(FFmpeg_FOUND)

    # avutil — base library everything else builds on; depends on nothing else here
    if(NOT TARGET FFmpeg::avutil)
        add_library(FFmpeg::avutil INTERFACE IMPORTED)
        set_target_properties(FFmpeg::avutil PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${AVUTIL_LIBRARY}"
        )
    endif()

    # swresample — audio resampling, depends on avutil
    if(NOT TARGET FFmpeg::swresample)
        add_library(FFmpeg::swresample INTERFACE IMPORTED)
        set_target_properties(FFmpeg::swresample PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SWRESAMPLE_LIBRARY};FFmpeg::avutil"
        )
    endif()

    # swscale — pixel format/scaling conversion, depends on avutil
    if(NOT TARGET FFmpeg::swscale)
        add_library(FFmpeg::swscale INTERFACE IMPORTED)
        set_target_properties(FFmpeg::swscale PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SWSCALE_LIBRARY};FFmpeg::avutil"
        )
    endif()

    # avcodec — encode/decode, depends on avutil
    if(NOT TARGET FFmpeg::avcodec)
        add_library(FFmpeg::avcodec INTERFACE IMPORTED)
        set_target_properties(FFmpeg::avcodec PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${AVCODEC_LIBRARY};FFmpeg::avutil"
        )
    endif()

    # avformat — muxing/demuxing, depends on avcodec + avutil
    if(NOT TARGET FFmpeg::avformat)
        add_library(FFmpeg::avformat INTERFACE IMPORTED)
        set_target_properties(FFmpeg::avformat PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${AVFORMAT_LIBRARY};FFmpeg::avcodec;FFmpeg::avutil"
        )
    endif()

endif()

mark_as_advanced(
    FFMPEG_INCLUDE_DIR
    AVUTIL_LIBRARY
    SWRESAMPLE_LIBRARY
    SWSCALE_LIBRARY
    AVCODEC_LIBRARY
    AVFORMAT_LIBRARY
)