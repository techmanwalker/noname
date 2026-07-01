# Module to find FFmpeg libraries
# Provides imported target FFmpeg::FFmpeg

# Find header paths
find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    DOC "Path to FFmpeg headers"
)

# Find each library individually
find_library(AVCODEC_LIBRARY NAMES avcodec)
find_library(AVFORMAT_LIBRARY NAMES avformat)
find_library(AVUTIL_LIBRARY NAMES avutil)
find_library(SWRESAMPLE_LIBRARY NAMES swresample)

# Verify that all the components exist
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_INCLUDE_DIR AVCODEC_LIBRARY AVFORMAT_LIBRARY AVUTIL_LIBRARY SWRESAMPLE_LIBRARY
)

# Create CMake target
if(FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    
    set_target_properties(FFmpeg::FFmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${AVCODEC_LIBRARY};${AVFORMAT_LIBRARY};${AVUTIL_LIBRARY};${SWRESAMPLE_LIBRARY}"
    )
endif()

# Hide internal variables
mark_as_advanced(
    FFMPEG_INCLUDE_DIR 
    AVCODEC_LIBRARY 
    AVFORMAT_LIBRARY 
    AVUTIL_LIBRARY 
    SWRESAMPLE_LIBRARY
)