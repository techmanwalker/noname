#pragma once

#include "soundio/backends.hpp"

#include "constexprstringcat.hpp"

// 1. Define your base descriptions
constexpr std::string_view prefix = "Target audio backend to connect via soundio\nPossible values: ";
constexpr std::string_view list = map::audio_backends_list;

// 2. Bake them together into static memory
inline constexpr auto complete_help_buffer = concat<prefix.size(), list.size()>(prefix, list);
inline constexpr std::string_view complete_help_view{complete_help_buffer.data(), complete_help_buffer.size() - 1};
