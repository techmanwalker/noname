#pragma once

#include "rolecompiler.hpp"

// compiled_role_set definitions

template <typename T>
compiled_role_set<T>::compiled_role_set(const role_definitions<T> &role_defs)
{
    int n = Qt::UserRole + 1;
    for (const auto &[name, extractor] : role_defs) {
        m_compiledroles.push_back({ n++, name, extractor });
    }
}

template <typename T>
QHash<int, QByteArray>
compiled_role_set<T>::role_names() const
{
    QHash<int, QByteArray> hash;
    for (const compiled_role<T> &r : m_compiledroles) {
        hash.insert(r.number, r.name);
    }
    return hash;
}

template <typename T>
std::optional<int> 
compiled_role_set<T>::role_number(const QByteArray &role) const
{
    for (const compiled_role<T> &r : m_compiledroles) {
        if (r.name == role) {
            return r.number;
        }
    }
    return std::nullopt;
}

template <typename T>
QVariant 
compiled_role_set<T>::extract(int role, const T &item) const
{
    for (const compiled_role<T> &r : m_compiledroles) {
        if (r.number == role) {
            return r.extractor(item);
        }
    }
    return {};
}

// Enable concatenation of role definition lists
template <typename T>
inline role_definitions<T>
operator+(
    const role_definitions<T> &a,
    const role_definitions<T> &b
)
{
    role_definitions<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}