#pragma once

#include "mediatypes.hpp"
#include "prettifiers.hpp"

#include <QVariant>
#include <QHash>

#include <optional>
#include <functional>
#include <vector>

/// Role definition. Enables automatic QML role generation for any data type.
template <typename T>
struct CompiledRole {
    int number;
    QByteArray name;
    std::function<QVariant(const T &)> extractor;
};

/**
    @brief Unidirectional, read-only callback that extracts a specific property from a container.
    
    @details This functional wrapper processes a read-only reference to a dynamic item (`const T&`).
    It projects the requested field into a QVariant returned by value.

    @note This extractor is strictly read-only. It provides a copy of the data, meaning it does not support 
    writing back to the source or bidirectional QML assignments through itself.
*/
template <typename T>
using RoleExtractor = std::function<QVariant(const T &)>;

/**
    @brief Gives the developer simplified controls to define roles.
*/
template <typename T>
using RoleDefinition  = std::pair<QByteArray, RoleExtractor<T>>;

template <typename T>
using RoleDefinitions = std::vector<RoleDefinition<T>>;

/**
    @brief Compiles a RoleDefinitions list into sequential Qt roles once, and answers
    the three questions every T-backed model needs: role → name, name → role,
    role → extracted value. 
*/
template <typename T>
class CompiledRoleSet
{
public:
    explicit CompiledRoleSet(const RoleDefinitions<T> &role_defs)
    {
        int n = Qt::UserRole + 1;
        for (const auto &[name, extractor] : role_defs) {
            m_compiledroles.push_back({ n++, name, extractor });
        }
    }

    QHash<int, QByteArray> roleNames() const
    {
        QHash<int, QByteArray> hash;
        for (const CompiledRole<T> &r : m_compiledroles) {
            hash.insert(r.number, r.name);
        }
        return hash;
    }

    std::optional<int> roleNumber(const QByteArray &role) const
    {
        for (const CompiledRole<T> &r : m_compiledroles) {
            if (r.name == role) {
                return r.number;
            }
        }
        return std::nullopt;
    }

    QVariant extract(int role, const T &item) const
    {
        for (const CompiledRole<T> &r : m_compiledroles) {
            if (r.number == role) {
                return r.extractor(item);
            }
        }
        return {};
    }

private:
    std::vector<CompiledRole<T>> m_compiledroles;
};

// Type trait to identify std::variant
template <typename T>
struct is_variant : std::false_type {};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};

// C++20 Concept
template <typename T>
concept IsVariant = is_variant<std::remove_cvref_t<T>>::value;

// Fully decoupled visitor
constexpr auto make_visitor = [](auto&& projector) {
    return [projector = std::forward<decltype(projector)>(projector)](const IsVariant auto &variant_instance) -> QVariant {
        return std::visit([&](const auto &x) -> QVariant { 
            return projector(x); 
        }, variant_instance);
    };
};

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

// Enable concatenation of role definition lists
template <typename T>
inline RoleDefinitions<T> operator+(
    const RoleDefinitions<T> &a,
    const RoleDefinitions<T> &b)
{
    RoleDefinitions<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}