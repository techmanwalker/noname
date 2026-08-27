#pragma once

#include "rolecompiler.hpp"

// CompiledRoleSet definitions

template <typename T>
CompiledRoleSet<T>::CompiledRoleSet(const RoleDefinitions<T> &role_defs)
{
    int n = Qt::UserRole + 1;
    for (const auto &[name, extractor] : role_defs) {
        m_compiledroles.push_back({ n++, name, extractor });
    }
}

template <typename T>
QHash<int, QByteArray>
CompiledRoleSet<T>::roleNames() const
{
    QHash<int, QByteArray> hash;
    for (const CompiledRole<T> &r : m_compiledroles) {
        hash.insert(r.number, r.name);
    }
    return hash;
}

template <typename T>
std::optional<int> 
CompiledRoleSet<T>::roleNumber(const QByteArray &role) const
{
    for (const CompiledRole<T> &r : m_compiledroles) {
        if (r.name == role) {
            return r.number;
        }
    }
    return std::nullopt;
}

template <typename T>
QVariant 
CompiledRoleSet<T>::extract(int role, const T &item) const
{
    for (const CompiledRole<T> &r : m_compiledroles) {
        if (r.number == role) {
            return r.extractor(item);
        }
    }
    return {};
}

// Enable concatenation of role definition lists
template <typename T>
inline RoleDefinitions<T>
operator+(
    const RoleDefinitions<T> &a,
    const RoleDefinitions<T> &b
)
{
    RoleDefinitions<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}