# Module to find libsoundio (https://sndio.org)

# Find headers path
find_path(SOUNDIO_INCLUDE_DIR
    NAMES soundio/soundio.h
    DOC "Path to libsoundio headers"
)

# libsoundio.so / libsoundio.a
find_library(SOUNDIO_LIBRARY
    NAMES soundio
    DOC "Path to libsoundio library"
)

# Check results
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SoundIo
    REQUIRED_VARS SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR
)

# Make CMake target to link
if(SOUNDIO_FOUND AND NOT TARGET SoundIo::SoundIo)
    add_library(SoundIo::SoundIo UNKNOWN IMPORTED)
    
    set_target_properties(SoundIo::SoundIo PROPERTIES
        IMPORTED_LOCATION "${SOUNDIO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SOUNDIO_INCLUDE_DIR}"
    )
endif()

# Hide internal variables
mark_as_advanced(SOUNDIO_INCLUDE_DIR SOUNDIO_LIBRARY)