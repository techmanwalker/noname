#include "abstractmodel.hpp"

AbstractModel::AbstractModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int
AbstractModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant
AbstractModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_items.size()))
        return {};

    const Types::Any &item = m_items[index.row()];

    // Roles common to both Song and Album
    switch (role) {
        case TypeRole:
            return static_cast<int>(item.index()); // 0=Song, 1=Album

        case TitleRole:
            return std::visit([](const auto &i) { return i.title; }, item);

        case ArtistRole:
            return std::visit([](const auto &i) { return i.artist; }, item);

        case CoverRole:
            return std::visit([](const auto &i) { return i.cover; }, item);

        case DurationRole:
            return std::visit([](const auto &i) -> qint64 {
                if constexpr (std::is_same_v<std::decay_t<decltype(i)>, Types::Song>)
                    return i.duration;
                else
                    return i.duration();
            }, item);

        default: return {};
    }
}

QHash<int, QByteArray>
AbstractModel::roleNames()
    const
{
    return {
        { TypeRole,     "type"     },
        { TitleRole,    "title"    },
        { ArtistRole,   "artist"   },
        { CoverRole,    "cover"    },
        { DurationRole, "duration" },
    };
}

void
AbstractModel::append(const Types::Any &item)
{
    int row = static_cast<int>(m_items.size());
    beginInsertRows({}, row, row);
    m_items.push_back(item);
    endInsertRows();
}

void
AbstractModel::remove(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    beginRemoveRows({}, index, index);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();
}

void
AbstractModel::clearItems()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const
Types::Any &AbstractModel::itemAt(int index) const
{
    return m_items.at(index);
}

int
AbstractModel::itemCount() const
{
    return static_cast<int>(m_items.size());
}