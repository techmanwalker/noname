#include "abstractmodel.hpp"

// Default role definitions for any AbstractModel


AbstractModel::AbstractModel(
    QObject *parent,
    RoleDefinitions role_defs
)
    : QAbstractListModel(parent),
      m_roledefs(std::move(role_defs))
{
    // Build the Roles based on the s_defs provided

    int n = Qt::UserRole + 1;
    for (auto &[name, extractor] : m_roledefs)
        m_compiledroles.push_back({ n++, name, std::move(extractor) });
}

int
AbstractModel::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant
AbstractModel::data(
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

QHash<int, QByteArray>
AbstractModel::roleNames() const
{
    QHash<int, QByteArray> hash;
    for (const CompiledRole &r : m_compiledroles)
        hash[r.number] = r.name;
    return hash;
}

void
AbstractModel::append(
    const Types::Any &item
)
{
    int row = static_cast<int>(m_items.size());
    beginInsertRows({}, row, row);
    m_items.push_back(item);
    endInsertRows();
}

void
AbstractModel::remove(
    int index
)
{
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    beginRemoveRows({}, index, index);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();
}

void
AbstractModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const
Types::Any &AbstractModel::itemAt(
    int index
) const
{
    return m_items.at(index);
}

int
AbstractModel::itemCount() const
{
    return static_cast<int>(m_items.size());
}