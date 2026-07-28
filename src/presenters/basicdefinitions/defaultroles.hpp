#pragma once

#include "mediatypes.hpp"

#include <QVariant>
#include <qvariant.h>

/// Role definition. Enables automatic QML role generation.
struct CompiledRole {
    int number;
    QByteArray name;
    std::function<QVariant(const Types::Any &)> extractor;
};

/**
    @brief Unidirectional, read-only callback that extracts a specific property from a media metadata container.
    
    @details This functional wrapper processes a read-only reference to a dynamic item (`const Types::Any&`),
    which can hold distinct underlying data types like Types::Song, Types::Album, or Types::Playlist.
    It inspects the variant's active type at runtime—typically using std::visit—and projects the requested 
    field into a QVariant returned by value.

    @note This extractor is strictly read-only. It provides a copy of the data, meaning it does not support 
    writing back to the source or bidirectional QML assignments through itself.
*/
using RoleExtractor = std::function<QVariant(const Types::Any &)>;

/**
    @brief Gives the developer simplified controls to define roles.
    A good example on how to do it is defaultroles.hpp.
*/
using RoleDefinition  = std::pair<QByteArray, RoleExtractor>;
using RoleDefinitions = std::vector<RoleDefinition>;



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
        if constexpr (std::is_same_v<T, Types::Directory>) {
            return QVariant{};
        } else {
            return x.cover;
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
            QList<Types::Song> sorted = x.songs;
            std::ranges::sort(sorted, [](const Types::Song &a, const Types::Song &b) {
                return a.title.localeAwareCompare(b.title) < 0;
            });
            return QVariant::fromValue(sorted);
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