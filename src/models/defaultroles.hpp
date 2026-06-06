#pragma once

#include "abstractmediasequence.hpp"
#include <QVariant>

// Helper to encapsulate the repetitive pattern of std::visit on the variant
static auto make_visitor = [](auto&& projector) {
    return [projector = std::forward<decltype(projector)>(projector)](const Types::Any &i) -> QVariant {
        return std::visit([&](const auto &x) -> QVariant { 
            return projector(x); 
        }, i);
    };
};

/**
    @brief Roles that are common to all container models.
    
    @details These are mostly basic getters/setters for each role. Defines a list of roles that
    are shared across most of models that inherit the Abstract Model.
*/
static const RoleDefinitions container_roles = {
    // Direct roles (thanks to the duck-typing of generic lambdas)
    { "title",  make_visitor([](const auto &x) { return x.title; }) },
    { "artist", make_visitor([](const auto &x) { return x.artist; }) },
    { "cover",  make_visitor([](const auto &x) { return x.cover; }) },

    // Conditional roles with specific logic
    { "album", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.album;
        } else {
            qDebug() << ".album is only defined for Songs themselves.";
            return QVariant{};
        }
    })},

    { "duration", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.duration;
        } else {
            return QVariant{};
        }
    })},

    { "type", make_visitor([](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>)     return "song";
        if constexpr (std::is_same_v<T, Types::Album>)    return "album";
        if constexpr (std::is_same_v<T, Types::Playlist>) return "playlist";
        return "unknown";
    })}
};

// Enable concatenation of role definition lists
inline RoleDefinitions operator+(
    const RoleDefinitions &a,
    const RoleDefinitions &b)
{
    RoleDefinitions result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}