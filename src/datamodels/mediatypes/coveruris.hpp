#pragma once

#include <string_view>

#include <QSize>

namespace covers {

// Address where cached covers are located
static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

// Default cover image uri
static constexpr char default_cover_uri[] = "";

// intended only to calculate storage costs
static constexpr QSize default_cover_rowthumb_size   {48, 48};
static constexpr QSize default_cover_card_size       {256, 256};
static constexpr QSize default_cover_background_size {910, 512}; // 16:9

}
