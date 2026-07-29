#include "abstractmediasequence.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qreadwritelock.h>
#include <rapidfuzz/fuzz.hpp>
#include <variant>

Q_LOGGING_CATEGORY(l_mediasequences, "noname.mediasequences")

// Default role definitions for any AbstractMediaSequence

/// Constructs the model and compiles the given role definitions into sequential Qt user roles.
AbstractMediaSequence::AbstractMediaSequence(
    QObject *parent,
    RoleDefinitions role_defs
)
    : QAbstractListModel(parent),
      m_roles(role_defs)
{
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

    return m_roles.extract(role, m_items[index.row()]);
}

/// Returns the role name map, allowing QML to resolve properties by their string names.
QHash<int, QByteArray>
AbstractMediaSequence::roleNames() const
{
    return m_roles.roleNames();
}

/// Reverse of roleNames(): resolves a role's string name back to its compiled int, if registered.
std::optional<int>
AbstractMediaSequence::roleNumber(const QByteArray &role) const
{
    return m_roles.roleNumber(role);
}

/// Reads a single role for a given row, for callers outside a delegate context.
QVariant
AbstractMediaSequence::readRole(qsizetype row, const QByteArray &role) const
{
    if (row < 0 || row >= rowCount()) return {};

    const std::optional<int> role_n = roleNumber(role);
    if (!role_n.has_value()) return {};

    return data(index(static_cast<int>(row)), role_n.value());
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
    if (index >= static_cast<int>(m_items.size())) return;
    beginRemoveRows({}, index, index);

    {
        QWriteLocker locker (&m_lock);
        m_items.erase(m_items.begin() + index);
    }

    endRemoveRows();

    emit countChanged();
}

QFuture<void>
AbstractMediaSequence::progressive_batch_append (QList<QFuture<Types::Any>> futures)
{
    QList<QFuture<void>> completion_signals;
    completion_signals.reserve(futures.size());

    for (QFuture<Types::Any> &pending : futures) {
        completion_signals.append(
            pending.then(this, [this](const Types::Any &result) {
                    // append() -> batch_append(Container) already filters empty-source
                    // Songs, so nothing extra needed here.
                    append(result);
                })
                .onFailed(this, [this] {
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

    for (const QPersistentModelIndex &item : items) {
        std::optional<size_t> prolly_exists = row_pointed_to(item);
        if (!prolly_exists.has_value()) continue;

        indices.push_back(prolly_exists.value());
    }

    if (indices.empty()) return;

    // 1. Sort descending so we can safely process chunks from the end of the container forward
    std::ranges::sort(indices, std::greater<size_t>());

    // 2. Linear scan to group and process contiguous intervals
    
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
        
        {
            QWriteLocker locker (&m_lock);
            /*  4. Erase the items from the underlying container for this chunk
                Since indices[i] through indices[j-1] are sorted descending, 
                erasing them in this order keeps the remaining indices valid. */
            for (size_t k = i; k < j; ++k) {
                m_items.removeAt(static_cast<int>(indices[k]));
            }
        }

        endRemoveRows();

        // Advance to the next non-contiguous chunk
        i = j;
    }

    emit countChanged();
}

void
AbstractMediaSequence::clear()
{
    beginResetModel();

    {
        QWriteLocker locker (&m_lock);
        m_items.clear();
    }

    endResetModel();

    emit countChanged();
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

const decltype(AbstractMediaSequence::m_items) &
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

std::string 
AbstractMediaSequence::normalize_string_for_search (const QString &str) 
{
    // Decompose string into base characters and separate diacritic marks
    QString normalized = str.normalized(QString::NormalizationForm_KD).toLower();
    
    // Efficiently strip out all non-spacing marks (the diacritics)
    normalized.removeIf([](QChar c) {
        return c.category() == QChar::Mark_NonSpacing;
    });
    
    return normalized.toStdString();
}

QList<QPersistentModelIndex>
AbstractMediaSequence::search_by_title (
    const QString &keywords, // search tokens, QString has wider locale support
    double score_thresh // out of 100
)
{

    // extracted from rapidfuzz README
    QReadLocker locker (&m_lock);

    // prepare to be ranked
    using rankable_item = std::pair<
            size_t, // list item index
            double // rapidfuzz score
        >;

    // rank result from scorer
    using rankable_list = std::vector<
        rankable_item
    >;
    
    rankable_list rankable;

    const std::string clean_keywords = normalize_string_for_search(keywords);
    rapidfuzz::fuzz::CachedPartialRatio<char> scorer (clean_keywords.c_str());

    for (size_t i = 0; i < m_items.size(); ++i) {

        // get the item title or name to compare
        const std::string clean_title = std::visit([](const auto& item) -> std::string {
            return normalize_string_for_search(item.title);
        }, m_items.at(i));

        // Apply the pointer-to-member operator (->*) to evaluate the target field
        double score = scorer.similarity(clean_title.c_str(), score_thresh);

        qCDebug(l_mediasequences) << "Searching for \"" << clean_keywords << "\", matching against " << clean_title
            << " scores " << score;

        if (score >= score_thresh) {
            rankable.emplace_back( // here
                i,
                score
            );
        }
    }

    // rank by scores in descending order
    std::ranges::sort (
        rankable, std::greater<>(), &rankable_item::second
    );

    QList<QPersistentModelIndex> ranked;
    ranked.reserve(rankable.size());

    for (const rankable_item &already_ranked : rankable) {
        // create persistent indices
        ranked.emplace_back(
            index(static_cast<int>(already_ranked.first))
        );
    }

    return ranked;
};