#pragma once

#include "mediatypes.hpp"
#include "prettifiers.hpp"
#include "rolecompiler.hpp"

#include <QVariant>
#include <QHash>
#include <qvariant.h>

/**
    @brief Roles that are common to all container models.
    
    @details These are mostly basic getters/setters for each role. Defines a list of roles that
    are shared across most of models that inherit the Abstract Model.
*/
static const RoleDefinitions<Types::Any> container_roles = {
    // Direct roles (thanks to the duck-typing of generic lambdas)
    { "title",  make_visitor([](const auto &x) {
        return x.title;
    })},
    { "artist",  make_visitor([](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Directory>) {
            return QVariant{};
        } else {
            return x.artist;
        }
    })},
    { "cover",  make_visitor([](const auto &x) {
        
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.cover.uri();
        } else {
            return QVariant{};
        }
    })},

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

    { "duration_mmss", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.duration_mmss();
        } else {
            return QVariant{};
        }
    })},

    { "printable_joint_metadata", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.printable_joint_metadata();
        } else {
            return QVariant{};
        }
    })},

    { "type", make_visitor([](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>)      return "Song";
        if constexpr (std::is_same_v<T, Types::Album>)     return "Album";
        if constexpr (std::is_same_v<T, Types::Playlist>)  return "Playlist";
        if constexpr (std::is_same_v<T, Types::Directory>) return "Directory";
        return "Unknown";
    })},

    {"songs", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, Types::Directory>) {
            // scan order is arbitrary — expose it sorted by title instead
            return QVariant::fromValue(
                Prettifiers::sortBy(
                    &Types::Song::title,
                    x.songs
                ));
        } else if constexpr (std::is_same_v<T, Types::Album>) { // Types::Playlist is the same type
            // track order / user-arranged order is meaningful here — leave it alone
            return QVariant::fromValue(x.songs);
        }

        return QVariant{};
    })},

    {"source", make_visitor([](const auto &x) -> QVariant {
        using T = std::decay_t<decltype(x)>;

        if constexpr (std::is_same_v<T, Types::Song>) {
            return x.source; // QUrl
        } else if constexpr (std::is_same_v<T, Types::Directory>) {
            return QUrl::fromLocalFile(x.path); // also QUrl
        }

        return QVariant{}; // here
    })}
};