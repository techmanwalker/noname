#pragma once

#include "mediatypes.hpp"
#include "rolecompiler.hpp"

#include <QVariant>
#include <QHash>

/**
    @brief Roles that are common to all container models.
    
    @details These are mostly basic getters/setters for each role. Defines a list of roles that
    are shared across most of models that inherit the Abstract Model.
*/
static const RoleDefinitions<Types::Any> container_roles = {
    /*  Whole-gadget role: lets a QML delegate declare a statically typed
    `required property song model` / `required property directory model`
    instead of dot-accessing the flattened per-field roles below.
    QVariant::fromValue(x) boxes whichever concrete alternative
    std::visit handed us — the QML engine then resolves it against
    the matching QML_VALUE_TYPE registration in mediatypes_qml.hpp
    purely from the QMetaType it carries. */
    { "model", make_visitor([](const auto &x) -> QVariant {
        return QVariant::fromValue(x);
    })},

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

        if constexpr (std::is_same_v<T, Types::Directory> || (std::is_same_v<T, Types::Album>)) {
            // already sorted as needed by each respective type
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