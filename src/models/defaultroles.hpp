#pragma once

#include "abstractmodel.hpp"
#include <QVariant>

// Roles that are common to all container models.
static const RoleDefinitions container_roles = {
    { "title",    [](const Types::Any &i) -> QVariant { return std::visit([](const auto &x) -> QVariant { return x.title; }, i); }},
    { "artist",   [](const Types::Any &i) -> QVariant { return std::visit([](const auto &x) -> QVariant { return x.artist; }, i); }},
    { "album", [](const Types::Any &i) -> QVariant {
        // if x is a song, return its album
        return std::visit([](const auto &x) -> QVariant {
            using T = std::decay_t<decltype(x)>; // whatever type X is

            if constexpr (std::is_same_v<T, Types::Song>)
                return x.album;
            else {
                qDebug() << ".album is only defined for Songs themselves.";
                return QVariant {};
            }
        }, i);
    }},

    { "cover",    [](const Types::Any &i) -> QVariant { return std::visit([](const auto &x) -> QVariant { return x.cover; }, i); }},
    { "duration", [](const Types::Any &i) -> QVariant {
        return std::visit([](const auto &x) -> QVariant {
            using T = std::decay_t<decltype(x)>; // whatever type X is
            
            if constexpr (std::is_same_v<T, Types::Song>)
                return x.duration;
            else
                // return x.duration(); when children can properly tell how long they are
                return QVariant {}; // undefined
        }, i);
    }},
    { "type", [](const Types::Any &i) -> QVariant {
        return std::visit([](const auto &x) -> QVariant {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, Types::Song>)  return "song";
            if constexpr (std::is_same_v<T, Types::Album>) return "album";
            if constexpr (std::is_same_v<T, Types::Playlist>) return "playlist";
            return "unknown";
        }, i);
    }}
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