#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

// Structural type (public members only), usable as a C++20 template argument.
// N includes the '\0'.
template<std::size_t N>
struct FixedString {
    char buf[N]{};

    constexpr FixedString(const char (&s)[N]) { std::copy_n(s, N, buf); }
};

// A and B each count their own '\0'; the result keeps only one.
template<std::size_t A, std::size_t B>
constexpr auto operator+(const FixedString<A>& a, const FixedString<B>& b)
{
    char out[A + B - 1]{};
    std::copy_n(a.buf, A - 1, out);
    std::copy_n(b.buf, B, out + A - 1);
    return FixedString<A + B - 1>{out};
}

// Helper function to concatenate views at compile time
template<size_t N1, size_t N2>
constexpr auto concat(std::string_view sv1, std::string_view sv2) {
    std::array<char, N1 + N2 + 1> result{};
    std::copy_n(sv1.data(), N1, result.data());
    std::copy_n(sv2.data(), N2, result.data() + N1);
    result[N1 + N2] = '\0';
    return result;
}
