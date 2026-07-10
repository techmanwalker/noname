#include "abstractmediasequence.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"
#include <algorithm>
#include <cstddef>
#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qreadwritelock.h>
#include <variant>

Q_LOGGING_CATEGORY(AbstractMediaSequence::l_mediasequences, "noname.mediasequences")

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
    QReadLocker locker (&m_lock);

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

QPersistentModelIndex
AbstractMediaSequence::append(
    const Types::Any &item
)
{
    // delegate and return its new persistent index
    return batch_append(QList<Types::Any> {item}).at(0);
}

void
AbstractMediaSequence::remove(
    size_t index
)
{
    // won't map to batch remove to avoid overhead
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;
    beginRemoveRows({}, index, index);
    QWriteLocker locker (&m_lock);
    m_items.erase(m_items.begin() + index);
    endRemoveRows();
}

#include <algorithm>
#include <functional>

void
AbstractMediaSequence::batch_remove(const QList<QPersistentModelIndex> &items)
{
    if (items.isEmpty()) return;

    std::vector<size_t> indices;
    indices.reserve(items.size());

    for (const QPersistentModelIndex &item : items) {
        std::optional<size_t> prolly_exists = row_pointed_to(item);
        if (!prolly_exists.has_value()) continue;

        indices.push_back(prolly_exists.value());
    }

    if (indices.empty()) return;

    // 1. Sort descending so we can safely process chunks from the end of the container forward
    std::ranges::sort(indices, std::greater<size_t>());

    // 2. Linear scan to group and process contiguous intervals

    QWriteLocker locker (&m_lock);
    
    size_t i = 0;
    while (i < indices.size()) {
        size_t last_idx = indices[i];  // The highest index in the current contiguous chunk
        size_t first_idx = last_idx;   // Will track the lowest index in this chunk

        // Look ahead to find where the contiguous block ends
        size_t j = i + 1;
        while (j < indices.size() && indices[j] == first_idx - 1) {
            first_idx = indices[j];
            j++;
        }

        // 3. Notify Qt about this specific contiguous sub-range
        beginRemoveRows(QModelIndex(), static_cast<int>(first_idx), static_cast<int>(last_idx));

        /*  4. Erase the items from the underlying container for this chunk
            Since indices[i] through indices[j-1] are sorted descending, 
            erasing them in this order keeps the remaining indices valid. */
        for (size_t k = i; k < j; ++k) {
            m_items.removeAt(static_cast<int>(indices[k]));
        }

        endRemoveRows();

        // Advance to the next non-contiguous chunk
        i = j;
    }
}

void
AbstractMediaSequence::clear()
{
    beginResetModel();
    QReadLocker locker (&m_lock);
    m_items.clear();
    endResetModel();
}

std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::item_at(
    size_t index
)
{
    QReadLocker locker (&m_lock);
    if (index >= static_cast<int>(m_items.size())) {
        return std::nullopt;
    };

    return std::ref(m_items[index]);
}

decltype(AbstractMediaSequence::m_items)
AbstractMediaSequence::items() const
{
    QReadLocker locker (&m_lock);
    return m_items;
}

// read write access to idx!
std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::pointed_to(const QPersistentModelIndex &idx)
{
    QReadLocker locker (&m_lock);

    const std::optional<size_t> row = __row_pointed_to_unlocked(idx);

    if (!row.has_value()) return std::nullopt;

    return std::ref(m_items[row.value()]);
}

std::optional<size_t>
AbstractMediaSequence::row_pointed_to(const QPersistentModelIndex &idx) const
{
    QReadLocker locker (&m_lock);

    const std::optional<size_t> row = __row_pointed_to_unlocked(idx);

    if (!row.has_value()) return std::nullopt;

    return std::ref(row.value());
}

std::optional<size_t>
AbstractMediaSequence::__row_pointed_to_unlocked(const QPersistentModelIndex &idx) const
{
    if (!idx.isValid()) {
        return std::nullopt;
    }

    const int row = idx.row();
    if (row < 0 || row >= static_cast<int>(m_items.size())) {
        return std::nullopt;
    }

    return row;
}

QStringList
AbstractMediaSequence::sources(const QList<Types::Any> &items) // Marked const assuming it doesn't modify the sequence
{
    QStringList uri_sources;

    // list everything that has a source or path
    
    // Pre-allocate memory for O(1) insertions to handle large sequences efficiently
    uri_sources.reserve(items.size());

    for (const Types::Any &item : std::as_const(items)) {
        std::visit([&uri_sources](const auto &resolved_item) { // [1]
            using T = std::decay_t<decltype(resolved_item)>;
            
            if constexpr (std::is_same_v<T, Types::Song>) {
                uri_sources.append(resolved_item.source.toLocalFile());
            }


        }, item);
    }

    return uri_sources;
}

QStringList
AbstractMediaSequence::sources () const
{
    return sources(m_items);
}

QFuture<void>
AbstractMediaSequence::batch_append (const QList<QUrl> &sources, std::shared_ptr<cover_provider> provider)
{
    return song_factory::batch_extract(sources, provider).then(this, [this, provider] (QList<Types::Song> result) {
        batch_append(result);
    });
}