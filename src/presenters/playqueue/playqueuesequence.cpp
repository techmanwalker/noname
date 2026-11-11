#include "playqueuesequence.hpp"

namespace sequence {

struct Access {
    static PlaylistSequence              &source (State &s) { return s.m_source; }
    static MediaTransforms::ShuffleProxy &view   (State &s) { return s.m_view; }
};

State::State()
{
    m_view.setSourceModel(&m_source);
}

QAbstractItemModel *
projection (State &s)
{
    return &Access::view(s);
}

void
on_count_changed (State &s, const QObject *context, std::function<void()> callback)
{
    QObject::connect(&Access::source(s), &PlaylistSequence::count_changed, context, std::move(callback));
}

// --- index translation

QModelIndex
to_source (State &s, const QModelIndex &projected)
{
    return Access::view(s).mapToSource(projected);
}

QPersistentModelIndex
to_projected (State &s, const QModelIndex &source)
{
    return QPersistentModelIndex(Access::view(s).mapFromSource(source));
}

QPersistentModelIndex
successor_of (State &s, const QModelIndex &projected)
{
    const MediaTransforms::ShuffleProxy &view = Access::view(s);
    if (!projected.isValid() || projected.row() + 1 >= view.rowCount()) return {};

    return QPersistentModelIndex(view.index(projected.row() + 1, 0));
}

// --- readers

bool
shuffled (State &s)
{
    return Access::view(s).shuffled();
}

int
item_count (State &s)
{
    return Access::source(s).rowCount();
}

QList<Types::Song>
items (State &s)
{
    QList<Types::Song> in_source_order = Access::source(s).items();

    const MediaTransforms::ShuffleProxy &view = Access::view(s);
    if (!view.shuffled()) return in_source_order;

    QList<Types::Song> presented;
    presented.reserve(in_source_order.size());
    for (int row = 0; row < view.rowCount(); ++row) {
        presented.append(in_source_order.at(view.mapToSource(view.index(row, 0)).row()));
    }
    return presented;
}

QPersistentModelIndex
find_by_source (State &s, const QUrl &source)
{
    return to_projected(s, Access::source(s).find(&Types::Song::source, source));
}

bool
pointed_to (State &s, const QPersistentModelIndex &projected,
            const std::function<bool(const Types::Song &)> &callback)
{
    // mapToSource is the identity while unshuffled and the permutation while shuffled,
    // so this one call is the source's pointed_to in both modes
    return Access::source(s).pointed_to<const Types::Song &>(
        QPersistentModelIndex(Access::view(s).mapToSource(projected)), callback);
}

// --- writers

void
clear (State &s)
{
    Access::source(s).clear();
}

void
batch_append (State &s, QList<Types::Song> songs)
{
    Access::source(s).batch_append(std::move(songs));
}

void
spawn (State &s, QList<Types::Song> songs)
{
    batch_append(s, std::move(songs));
    Access::view(s).reshuffle(); // no-op while unshuffled
}

bool
move_item (State &s, const QPersistentModelIndex &src, const QPersistentModelIndex &dest)
{
    if (Access::view(s).shuffled()) {
        // only the shuffled view moves; the source keeps its order
        return Access::view(s).move_presented(src, dest);
    }

    // unshuffled: the projection is the source, so this is the real move
    return Access::source(s).move_item(to_source(s, src), to_source(s, dest));
}

void
batch_remove (State &s, const QList<QPersistentModelIndex> &source_indices)
{
    Access::source(s).batch_remove(source_indices);
}

void
set_shuffled (State &s, bool enabled, const QPersistentModelIndex &lead)
{
    Access::view(s).set_shuffled(enabled, lead);
}

}
