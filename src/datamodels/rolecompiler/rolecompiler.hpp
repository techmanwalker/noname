#pragma once

#include <QByteArray>
#include <QVariant>

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
    // pipe your custom role definitions to this to get your compiled roles ready to use
    explicit CompiledRoleSet(const RoleDefinitions<T> &role_defs);

    // get your newly compiled role names
    QHash<int, QByteArray> roleNames() const;

    // get the role number corresponding to this name
    std::optional<int> roleNumber(const QByteArray &role) const;

    // get the value contained by this role
    QVariant extract(int role, const T &item) const;

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

// Enable concatenation of role definition lists
template <typename T>
inline RoleDefinitions<T> operator+(
    const RoleDefinitions<T> &a,
    const RoleDefinitions<T> &b);

#include "rolecompiler.tpp"