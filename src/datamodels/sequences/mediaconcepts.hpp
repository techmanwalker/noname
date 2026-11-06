#pragma once

#include <concepts>

#include "mediatypes.hpp"

template <typename T>
concept media_sequence = 
    std::ranges::forward_range<T> && 
    media_type<typename T::value_type>;

template <typename... Ts>
struct all_distinct : std::true_type {};

template <typename T, typename... Rest>
struct all_distinct<T, Rest...>
    : std::bool_constant<(!std::same_as<T, Rest> && ...) && all_distinct<Rest...>::value> {};

template <typename... Ts>
inline constexpr bool all_distinct_v = all_distinct<Ts...>::value;

template <typename... Ts>
concept distinct_media_types =
    (sizeof...(Ts) > 0)
    && (media_type<Ts> && ...)
    && all_distinct_v<Ts...>;