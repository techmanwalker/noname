#pragma once

#include <cstddef>

struct attributes {
    size_t crop_and_resize = 0; // if non-zero, song_factory will crop the center of the artwork and rescale it to NxN to save memory
    bool   use_thumbnail_cache = true; // if true, check localdata's on-disk thumbnail cache before decoding cover art from the audio file
};