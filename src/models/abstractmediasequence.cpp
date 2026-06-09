#include "abstractmediasequence.hpp"

// Default role definitions for any AbstractMediaSequence

/// Constructs the model and compiles the given role definitions into sequential Qt user roles.
AbstractMediaSequence::AbstractMediaSequence(
    QObject *parent,
    RoleDefinitions role_defs
)
    : QAbstractListModel(parent),
      m_roledefs(std::move(role_defs))
{
    // Build the Roles based on the role_defs provided

    int n = Qt::UserRole + 1;
    for (auto &[name, extractor] : m_roledefs)
        m_compiledroles.push_back({ n++, name, std::move(extractor) });
}

// Count items (intended for QML)
int
AbstractMediaSequence::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

// Count items (intended for C++)
int
AbstractMediaSequence::itemCount() const
{
    return static_cast<int>(m_items.size());
}

/// Retrieves the value of a specific role for the item at the given index
QVariant
AbstractMediaSequence::data(
    const QModelIndex &index,
    int role
) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_items.size()))
        return {};

    const Types::Any &item = m_items[index.row()];

    for (const CompiledRole &r : m_compiledroles)
        if (r.number == role)
            return r.extractor(item);

    return {};
}

/// Returns the role name map, allowing QML to resolve properties by their string names.
QHash<int, QByteArray>
AbstractMediaSequence::roleNames() const
{
    QHash<int, QByteArray> hash;
    for (const CompiledRole &r : m_compiledroles)
        hash[r.number] = r.name;
    return hash;
}

void
AbstractMediaSequence::append(
    const Types::Any &item
)
{
    // delegate
    batch_append(QList<Types::Any> {item});
}

void
AbstractMediaSequence::remove(
    int index
)
{
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    beginRemoveRows({}, index, index);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();
}

void
AbstractMediaSequence::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const
Types::Any &AbstractMediaSequence::itemAt(
    int index
) const
{
    return m_items.at(index);
}

QList<Types::Any>
AbstractMediaSequence::items() const
{
    return m_items;
}