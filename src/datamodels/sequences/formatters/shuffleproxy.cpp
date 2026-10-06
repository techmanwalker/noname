#include "shuffleproxy.hpp"

#include <algorithm>
#include <functional>
#include <numeric>

ShuffleProxy::ShuffleProxy(QObject *parent) : QAbstractProxyModel(parent) {}

void
ShuffleProxy::setSourceModel(QAbstractItemModel *source)
{
    beginResetModel();

    for (const QMetaObject::Connection &c : std::as_const(m_connections)) disconnect(c);
    m_connections.clear();

    QAbstractProxyModel::setSourceModel(source);

    // QAbstractProxyModel forwards nothing from the source: every signal that matters is wired here
    if (source) {
        m_connections = {
            connect(source, &QAbstractItemModel::rowsAboutToBeInserted, this, &ShuffleProxy::on_source_rows_about_to_be_inserted),
            connect(source, &QAbstractItemModel::rowsInserted,          this, &ShuffleProxy::on_source_rows_inserted),
            connect(source, &QAbstractItemModel::rowsAboutToBeRemoved,  this, &ShuffleProxy::on_source_rows_about_to_be_removed),
            connect(source, &QAbstractItemModel::rowsRemoved,           this, &ShuffleProxy::on_source_rows_removed),
            connect(source, &QAbstractItemModel::dataChanged,           this, &ShuffleProxy::on_source_data_changed),
            connect(source, &QAbstractItemModel::modelAboutToBeReset,   this, [this] { beginResetModel(); }),
            connect(source, &QAbstractItemModel::modelReset,            this, [this] { endResetModel(); }), // -> resetInternalData() rebuilds from the new source
        };
    }

    endResetModel(); // -> resetInternalData() builds the order
    // Not handled: layoutChanged / rowsMoved. AbstractMediaSequence never emits them.
}

void ShuffleProxy::resetInternalData()
{
    QAbstractProxyModel::resetInternalData();
    rebuild_order();
}

void
ShuffleProxy::rebuild_order(int pinned_source_row)
{
    const int n = sourceModel() ? sourceModel()->rowCount() : 0;

    m_order.clear();
    if (m_shuffled) {
        m_order.resize(n);
        std::iota(m_order.begin(), m_order.end(), 0);
        std::shuffle(m_order.begin(), m_order.end(), m_rng);

        if (pinned_source_row >= 0 && pinned_source_row < n) {
            std::iter_swap(m_order.begin(), std::ranges::find(m_order, pinned_source_row));
        }
    }
    rebuild_rank();
}

void
ShuffleProxy::rebuild_rank()
{
    m_rank.clear();
    if (!m_shuffled || !sourceModel()) return;

    m_rank.assign(sourceModel()->rowCount(), -1);
    for (int p = 0; p < static_cast<int>(m_order.size()); ++p) {
        if (m_order[p] < static_cast<int>(m_rank.size())) m_rank[m_order[p]] = p;
    }
}

void
ShuffleProxy::setShuffled(bool enabled, int pinned_source_row)
{
    if (enabled == m_shuffled) return;

    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);

    // remember where every persistent index points in the *source* ...
    const QModelIndexList stale = persistentIndexList();
    QModelIndexList sources;
    sources.reserve(stale.size());
    for (const QModelIndex &p : stale) sources.append(mapToSource(p));

    m_shuffled = enabled;
    rebuild_order(pinned_source_row);

    // ... and re-resolve them under the new projection
    QModelIndexList fresh;
    fresh.reserve(stale.size());
    for (const QModelIndex &s : std::as_const(sources)) fresh.append(mapFromSource(s));
    changePersistentIndexList(stale, fresh);

    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
}

QModelIndex
ShuffleProxy::mapToSource(const QModelIndex &proxy) const
{
    if (!sourceModel() || !proxy.isValid()) return {};

    const int row = proxy.row();
    if (!m_shuffled) return sourceModel()->index(row, proxy.column());
    if (row < 0 || row >= static_cast<int>(m_order.size())) return {};

    return sourceModel()->index(m_order[row], proxy.column());
}

QModelIndex
ShuffleProxy::mapFromSource(const QModelIndex &source) const
{
    if (!sourceModel() || !source.isValid()) return {};

    const int row = source.row();
    if (!m_shuffled) return index(row, source.column());
    if (row < 0 || row >= static_cast<int>(m_rank.size()) || m_rank[row] < 0) return {};

    return index(m_rank[row], source.column());
}

QModelIndex
ShuffleProxy::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

int
ShuffleProxy::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !sourceModel()) return 0;
    return m_shuffled ? static_cast<int>(m_order.size()) : sourceModel()->rowCount();
}

int
ShuffleProxy::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid() || !sourceModel()) ? 0 : sourceModel()->columnCount();
}

void
ShuffleProxy::on_source_rows_about_to_be_inserted(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid()) return;

    Q_ASSERT(!m_shuffled || static_cast<int>(m_order.size()) == sourceModel()->rowCount());

    if (!m_shuffled) {
        beginInsertRows({}, first, last);
        return;
    }

    // new rows always land at the tail of the presented order
    const int tail = static_cast<int>(m_order.size());
    beginInsertRows({}, tail, tail + (last - first));
}

void
ShuffleProxy::on_source_rows_inserted(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid()) return;

    if (m_shuffled) {
        const int count = last - first + 1;

        for (int &source_row : m_order) {
            if (source_row >= first) source_row += count;
        }

        const auto tail = m_order.insert(m_order.end(), count, 0);
        std::iota(tail, m_order.end(), first);
        std::shuffle(tail, m_order.end(), m_rng); // shuffled among themselves

        rebuild_rank();
    }

    endInsertRows();
}

void
ShuffleProxy::on_source_rows_about_to_be_removed(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid()) return;

    Q_ASSERT(!m_shuffled || static_cast<int>(m_order.size()) == sourceModel()->rowCount());

    if (!m_shuffled) {
        beginRemoveRows({}, first, last);
        return;
    }

    // A contiguous source range is scattered in presented order: one removal
    // per row, highest proxy row first so the remaining ones don't shift.
    std::vector<int> doomed;
    for (int s = first; s <= last; ++s) doomed.push_back(m_rank[s]);
    std::ranges::sort(doomed, std::greater<>{});

    for (int p : doomed) {
        beginRemoveRows({}, p, p);
        m_order.erase(m_order.begin() + p);
        rebuild_rank();
        endRemoveRows();
    }
}

void
ShuffleProxy::on_source_rows_removed(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid()) return;

    if (!m_shuffled) {
        endRemoveRows();
        return;
    }

    // rows already left the proxy; now the source numbering closes the gap
    const int count = last - first + 1;
    for (int &source_row : m_order) {
        if (source_row > last) source_row -= count;
    }
    rebuild_rank();
}

void
ShuffleProxy::on_source_data_changed(const QModelIndex &top_left, const QModelIndex &bottom_right, const QList<int> &roles)
{
    // contiguous in the source, scattered when presented: one signal per row
    for (int row = top_left.row(); row <= bottom_right.row(); ++row) {
        const QModelIndex changed = mapFromSource(sourceModel()->index(row, top_left.column()));
        if (changed.isValid()) emit dataChanged(changed, changed, roles);
    }
}