#pragma once

#include <string_view>

namespace covers {

// Address where cached covers are located
static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

// Default cover image uri
static constexpr char default_cover_uri[] = "";

static constexpr size_t default_cover_thumbnail_size = 256;

}