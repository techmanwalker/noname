#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "soundio/backends.hpp"

// A structural type that can be passed as a template argument in C++20
template<size_t N>
struct FixedString {
    std::array<char, N> buf{};
    
    constexpr FixedString(std::string_view sv) {
        std::copy_n(sv.data(), sv.size(), buf.data());
    }
    
    constexpr const char* data() const { return buf.data(); }
};

// Helper function to concatenate views at compile time
template<size_t N1, size_t N2>
constexpr auto concat(std::string_view sv1, std::string_view sv2) {
    std::array<char, N1 + N2 + 1> result{};
    std::copy_n(sv1.data(), N1, result.data());
    std::copy_n(sv2.data(), N2, result.data() + N1);
    result[N1 + N2] = '\0';
    return result;
}

// 1. Define your base descriptions
constexpr std::string_view prefix = "Target audio backend to connect via soundio\nPossible values: ";
constexpr std::string_view list = map::audio_backends_list;

// 2. Bake them together into static memory
inline constexpr auto complete_help_buffer = concat<prefix.size(), list.size()>(prefix, list);
inline constexpr std::string_view complete_help_view{complete_help_buffer.data(), complete_help_buffer.size() - 1};
