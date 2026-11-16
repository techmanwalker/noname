#include "abstractmediasequence.hpp"
#include "rolecompiler.hpp" // Isolated completely to the implementation
#include "mediatypes.hpp"

#include "sort.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

Q_LOGGING_CATEGORY(l_mediasequences, "mediasequences")

class AbstractMediaSequencePrivate {
public:
    AbstractMediaSequencePrivate(const std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> &role_defs)
        : m_roles(role_defs) {}

    compiled_role_set<Types::Any> m_roles;
    QList<Types::Any> m_items;
    mutable QReadWriteLock m_lock;
};

AbstractMediaSequence::AbstractMediaSequence(
    QObject *parent,
    const std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> &role_defs
)
    : QAbstractListModel(parent),
      m_d(std::make_unique<AbstractMediaSequencePrivate>(role_defs))
{
}

// Ensure the standard unique_ptr destructor handles incomplete types correctly
AbstractMediaSequence::~AbstractMediaSequence() = default;

// Accessors for .tpp file logic
QList<Types::Any>& AbstractMediaSequence::_items() { return m_d->m_items; }
const QList<Types::Any>& AbstractMediaSequence::_items() const { return m_d->m_items; }
QReadWriteLock& AbstractMediaSequence::_lock() const { return m_d->m_lock; }

int
AbstractMediaSequence::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid()) return 0;
    QReadLocker locker (&m_d->m_lock);
    return static_cast<int>(m_d->m_items.size());
}

QVariant
AbstractMediaSequence::data(
    const QModelIndex &index,
    int role
) const
{
    QReadLocker locker (&m_d->m_lock);

    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_d->m_items.size()))
        return {};

    return m_d->m_roles.extract(role, m_d->m_items[index.row()]);
}

QHash<int, QByteArray>
AbstractMediaSequence::roleNames() const
{
    return m_d->m_roles.role_names();
}

std::optional<int>
AbstractMediaSequence::role_number(const QByteArray &role) const
{
    return m_d->m_roles.role_number(role);
}

QVariant
AbstractMediaSequence::read_role(qsizetype row, const QByteArray &role) const
{
    if (row < 0 || row >= rowCount()) return {};

    const std::optional<int> role_n = role_number(role);
    if (!role_n.has_value()) return {};

    return data(index(static_cast<int>(row)), role_n.value());
}

void
AbstractMediaSequence::remove(size_t index)
{
    if (static_cast<qsizetype>(index) >= rowCount()) return;
    beginRemoveRows({}, static_cast<int>(index), static_cast<int>(index));

    {
        QWriteLocker locker (&m_d->m_lock);
        m_d->m_items.erase(m_d->m_items.begin() + static_cast<int>(index));
    }

    endRemoveRows();

    emit count_changed();
}

QFuture<void>
AbstractMediaSequence::progressive_batch_append (QList<QFuture<Types::Any>> futures)
{
    QList<QFuture<void>> completion_signals;
    completion_signals.reserve(futures.size());

    for (QFuture<Types::Any> &pending : futures) {
        completion_signals.emplace_back(
            pending.then(this, [this](Types::Any result) {
                    append(std::move(result));
                })
                .onFailed(this, [] {
                    qCWarning(l_mediasequences) << "Progressive batch append: one item's future failed; skipping it.";
                })
        );
    }

    return QtFuture::whenAll(completion_signals.begin(), completion_signals.end());
}

void
AbstractMediaSequence::batch_remove(const QList<QPersistentModelIndex> &items)
{
    if (items.isEmpty()) return;

    std::vector<size_t> indices;
    indices.reserve(items.size());

    const int count = rowCount();
    for (const QPersistentModelIndex &item : items) {
        if (!item.isValid()) continue;
        const int row = item.row();
        if (row < 0 || row >= count) continue;
        indices.push_back(static_cast<size_t>(row));
    }

    if (indices.empty()) return;

    // descending, deduplicated: the run-merging loop below relies on both
    std::ranges::sort(indices, std::greater<size_t>());
    const auto dupes = std::ranges::unique(indices);
    indices.erase(dupes.begin(), dupes.end());

    size_t i = 0;
    while (i < indices.size()) {
        size_t last_idx = indices[i];
        size_t first_idx = last_idx;

        size_t j = i + 1;
        while (j < indices.size() && indices[j] == first_idx - 1) {
            first_idx = indices[j];
            j++;
        }

        beginRemoveRows(QModelIndex(), static_cast<int>(first_idx), static_cast<int>(last_idx));

        {
            QWriteLocker locker (&m_d->m_lock);
            for (size_t k = i; k < j; ++k) {
                m_d->m_items.removeAt(static_cast<int>(indices[k]));
            }
        }

        endRemoveRows();
        i = j;
    }

    emit count_changed();
}

void
AbstractMediaSequence::clear()
{
    beginResetModel();
    {
        QWriteLocker locker (&m_d->m_lock);
        m_d->m_items.clear();
    }
    endResetModel();
    emit count_changed();
}

void
AbstractMediaSequence::sort_rows (const SortProjection &project, bool descending)
{
    std::vector<qsizetype> new_row_of; // old row -> new row

    emit layoutAboutToBeChanged();

    {
        QWriteLocker locker (&m_d->m_lock);
        QList<Types::Any> &items = m_d->m_items;

        std::vector<std::pair<qsizetype, SortKey>> keyed;
        for (qsizetype row = 0; row < items.size(); ++row) {
            if (std::optional<SortKey> key = project(items.at(row)))
                keyed.emplace_back(row, std::move(*key));
        }

        // rows holding a sortable item: the only ones that get rewritten
        std::vector<qsizetype> target_rows;
        target_rows.reserve(keyed.size());
        for (const auto &entry : keyed) target_rows.push_back(entry.first);

        std::ranges::stable_sort(keyed, [descending](const auto &a, const auto &b) {
            // one projection per call: both keys always hold the same alternative
            return std::visit([&]<typename K>(const K &lhs) {
                return MediaTransforms::field_less(lhs, std::get<K>(b.second), descending);
            }, a.second);
        });

        new_row_of.resize(items.size());
        std::iota(new_row_of.begin(), new_row_of.end(), qsizetype{0});

        QList<Types::Any> reordered = items;
        for (size_t i = 0; i < keyed.size(); ++i) {
            reordered[target_rows[i]] = items.at(keyed[i].first);
            new_row_of[keyed[i].first] = target_rows[i];
        }
        items = std::move(reordered);
    }

    // createIndex, not index(): index() takes the read lock via rowCount()
    const QModelIndexList from = persistentIndexList();
    QModelIndexList to;
    to.reserve(from.size());
    for (const QModelIndex &idx : from)
        to.append(createIndex(static_cast<int>(new_row_of[idx.row()]), idx.column()));
    changePersistentIndexList(from, to);

    emit layoutChanged();
}

bool
AbstractMediaSequence::move_item(
    const QPersistentModelIndex &src,
    const QPersistentModelIndex &dest
)
{
    // also rejects invalid indices: an invalid QPersistentModelIndex has no model
    if (src.model() != this || dest.model() != this) return false;

    int from = 0;
    int to   = 0;
    {
        QReadLocker locker (&m_d->m_lock);

        const std::optional<size_t> src_row  = __row_pointed_to_unlocked(src);
        const std::optional<size_t> dest_row = __row_pointed_to_unlocked(dest);
        if (!src_row.has_value() || !dest_row.has_value()) return false;

        from = static_cast<int>(src_row.value());
        to   = static_cast<int>(dest_row.value());
    }

    // nothing to move, but not a failure
    if (from == to) return true;

    // Qt's destinationChild means "insert before this row" in pre-move numbering,
    // so moving downwards must target the row after dest to land exactly on it.
    const int destination_child = (from < to) ? to + 1 : to;

    // The rows were read above, before this call: the persistent indices
    // are rewritten by Qt as part of endMoveRows().
    if (!beginMoveRows({}, from, from, {}, destination_child)) return false;

    {
        QWriteLocker locker (&m_d->m_lock);
        m_d->m_items.move(from, to);
    }

    endMoveRows();
    return true;
}

QList<Types::Any>
AbstractMediaSequence::items() const
{
    QReadLocker locker (&m_d->m_lock);
    return m_d->m_items;
}

std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::__item_pointed_to_unlocked (const QPersistentModelIndex &idx) const
{
    const std::optional<size_t> row = __row_pointed_to_unlocked(idx);

    if (!row.has_value()) return std::nullopt;

    return std::ref(m_d->m_items[static_cast<qsizetype>(row.value())]);
}

std::optional<size_t>
AbstractMediaSequence::__row_pointed_to_unlocked(const QPersistentModelIndex &idx) const
{
    if (!idx.isValid()) {
        return std::nullopt;
    }

    const int row = idx.row();
    if (row < 0 || row >= static_cast<int>(m_d->m_items.size())) {
        return std::nullopt;
    }

    return row;
}

QStringList
AbstractMediaSequence::sources () const
{
    QReadLocker locker (&m_d->m_lock);

    // read sources for the private items list as is
    return sources<>(m_d->m_items);
}
