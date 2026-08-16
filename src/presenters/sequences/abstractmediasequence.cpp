#include "abstractmediasequence.hpp"
#include "rolecompiler.hpp" // Isolated completely to the implementation
#include "mediatypes.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <rapidfuzz/fuzz.hpp>
#include <variant>

Q_LOGGING_CATEGORY(l_mediasequences, "noname.mediasequences")

class AbstractMediaSequencePrivate {
public:
    AbstractMediaSequencePrivate(const std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> &role_defs)
        : m_roles(role_defs) {}

    CompiledRoleSet<Types::Any> m_roles;
    QList<Types::Any> m_items;
    mutable QReadWriteLock m_lock;
};

AbstractMediaSequence::AbstractMediaSequence(
    QObject *parent,
    std::vector<std::pair<QByteArray, std::function<QVariant(const Types::Any &)>>> role_defs
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
    return static_cast<int>(m_d->m_items.size());
}

int
AbstractMediaSequence::itemCount() const
{
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
    return m_d->m_roles.roleNames();
}

std::optional<int>
AbstractMediaSequence::roleNumber(const QByteArray &role) const
{
    return m_d->m_roles.roleNumber(role);
}

QVariant
AbstractMediaSequence::readRole(qsizetype row, const QByteArray &role) const
{
    if (row < 0 || row >= rowCount()) return {};

    const std::optional<int> role_n = roleNumber(role);
    if (!role_n.has_value()) return {};

    return data(index(static_cast<int>(row)), role_n.value());
}

QPersistentModelIndex
AbstractMediaSequence::append(const Types::Any &item)
{
    return batch_append(QList<Types::Any> {item}).at(0);
}

void
AbstractMediaSequence::remove(size_t index)
{
    if (index >= static_cast<int>(m_d->m_items.size())) return;
    beginRemoveRows({}, index, index);

    {
        QWriteLocker locker (&m_d->m_lock);
        m_d->m_items.erase(m_d->m_items.begin() + index);
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

    std::ranges::sort(indices, std::greater<size_t>());
    
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

    emit countChanged();
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
    emit countChanged();
}

std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::item_at(size_t index)
{
    QReadLocker locker (&m_d->m_lock);
    if (index >= static_cast<int>(m_d->m_items.size())) {
        return std::nullopt;
    };

    return std::ref(m_d->m_items[index]);
}

const QList<Types::Any> &
AbstractMediaSequence::items() const
{
    QReadLocker locker (&m_d->m_lock);
    return m_d->m_items;
}

std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::pointed_to(const QPersistentModelIndex &idx)
{
    QReadLocker locker (&m_d->m_lock);

    const std::optional<size_t> row = __row_pointed_to_unlocked(idx);

    if (!row.has_value()) return std::nullopt;

    return std::ref(m_d->m_items[row.value()]);
}

std::optional<std::reference_wrapper<Types::Any>>
AbstractMediaSequence::next_to(const QPersistentModelIndex &idx)
{
    QReadLocker locker (&m_d->m_lock);

    const QPersistentModelIndex next_idx = __index_next_to_unlocked(idx);

    if (!next_idx.isValid()) return std::nullopt;

    return pointed_to(next_idx);
}

std::optional<size_t>
AbstractMediaSequence::row_pointed_to(const QPersistentModelIndex &idx) const
{
    QReadLocker locker (&m_d->m_lock);

    const std::optional<size_t> row = __row_pointed_to_unlocked(idx);

    if (!row.has_value()) return std::nullopt;

    return std::ref(row.value());
}

QPersistentModelIndex 
AbstractMediaSequence::index_next_to (const QPersistentModelIndex &idx) const
{
    QReadLocker locker (&m_d->m_lock);
    return __index_next_to_unlocked(idx);
}

std::optional<size_t>
AbstractMediaSequence::row_next_to(const QPersistentModelIndex &idx) const
{
    QReadLocker locker (&m_d->m_lock);

    const std::optional<size_t> next_row = __row_next_to_unlocked(idx);

    if (!next_row.has_value()) return std::nullopt;

    return next_row.value();
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

std::optional<size_t>
AbstractMediaSequence::__row_next_to_unlocked(const QPersistentModelIndex &idx) const
{
    if (!idx.isValid()) {
        return std::nullopt;
    }

    const int row = idx.row() + 1;

    if (row < 1 || row >= static_cast<int>(m_d->m_items.size())) {
        return std::nullopt;
    }

    return row;
}

QPersistentModelIndex
AbstractMediaSequence::__index_next_to_unlocked (const QPersistentModelIndex &idx) const
{
    if (!idx.isValid()) {
        return QPersistentModelIndex();
    }

    const std::optional<size_t> next_row = __row_next_to_unlocked(idx);

    if (!next_row.has_value()) return QPersistentModelIndex();

    return index(next_row.value());
}

QStringList
AbstractMediaSequence::sources () const
{
    return sources<QList<Types::Any>>(m_d->m_items);
}

std::string 
AbstractMediaSequence::normalize_string_for_search (const QString &str) 
{
    QString normalized = str.normalized(QString::NormalizationForm_KD).toLower();
    
    normalized.removeIf([](QChar c) {
        return c.category() == QChar::Mark_NonSpacing;
    });
    
    return normalized.toStdString();
}

QList<QPersistentModelIndex>
AbstractMediaSequence::search_by_title (
    const QString &keywords,
    double score_thresh
)
{
    QReadLocker locker (&m_d->m_lock);

    using rankable_item = std::pair<size_t, double>;
    using rankable_list = std::vector<rankable_item>;
    
    rankable_list rankable;

    const std::string clean_keywords = normalize_string_for_search(keywords);
    rapidfuzz::fuzz::CachedPartialRatio<char> scorer (clean_keywords.c_str());

    for (size_t i = 0; i < m_d->m_items.size(); ++i) {

        const std::string clean_title = std::visit([](const auto& item) -> std::string {
            return normalize_string_for_search(item.title);
        }, m_d->m_items.at(i));

        double score = scorer.similarity(clean_title.c_str(), score_thresh);

        qCDebug(l_mediasequences) << "Searching for \"" << clean_keywords << "\", matching against " << clean_title
            << " scores " << score;

        if (score >= score_thresh) {
            rankable.emplace_back(i, score);
        }
    }

    std::ranges::sort (
        rankable, std::greater<>(), &rankable_item::second
    );

    QList<QPersistentModelIndex> ranked;
    ranked.reserve(rankable.size());

    for (const rankable_item &already_ranked : rankable) {
        ranked.emplace_back(
            index(static_cast<int>(already_ranked.first))
        );
    }

    return ranked;
};