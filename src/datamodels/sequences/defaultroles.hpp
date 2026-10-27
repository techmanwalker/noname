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
static const role_definitions<Types::Any> container_roles = {

    // Default roles that return a gadget.
    { "model", make_visitor([](const auto &x) -> QVariant {
        return QVariant::fromValue(x);
    })},

    // Injects 'modelData' into the QML delegate scope, matching raw list behavior
    { "modelData", make_visitor([](const auto &x) -> QVariant {
        return QVariant::fromValue(x);
    })},

    // reflection still missing
    { "type", make_visitor([](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, Types::Song>)      return "Song";
        if constexpr (std::is_same_v<T, Types::Album>)     return "Album";
        if constexpr (std::is_same_v<T, Types::Playlist>)  return "Playlist";
        if constexpr (std::is_same_v<T, Types::Directory>) return "Directory";
        return "Unknown";
    })}
};