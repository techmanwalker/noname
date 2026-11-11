#pragma once

// The switch between the two lists behind the play queue.
//
//   source         the original list, in insertion order
//   shuffled view  the source seen through ShuffleProxy
//   projected      what the UI shows: the source while unshuffled, the shuffled view otherwise
//
// Only this namespace touches the models. Structure changes (append, remove, clear) go to the
// source and the shuffled view follows by reacting to the source's signals; a move touches only
// the list being projected, so the source stays pristine while shuffled.
// Indices taken and returned here are in the projected space (ShuffleProxy's), unless a
// parameter says "source".

#include "mediatypes.hpp"
#include "playlistsequence.hpp"
#include "shuffleproxy.hpp"

#include <QList>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QUrl>

#include <functional>

namespace sequence {

class State
{
public:
    State();

private:
    friend struct Access; // defined in playqueuesequence.cpp: the only code allowed in here

    PlaylistSequence              m_source;
    MediaTransforms::ShuffleProxy m_view;
};

// the model the queue presents; PlayQueueLI is an identity proxy over it
QAbstractItemModel *projection (State &s);

void on_count_changed (State &s, const QObject *context, std::function<void()> callback);

// --- index translation
QModelIndex           to_source    (State &s, const QModelIndex &projected);
QPersistentModelIndex to_projected (State &s, const QModelIndex &source);
QPersistentModelIndex successor_of (State &s, const QModelIndex &projected); // in the projected order

// --- readers
bool                  shuffled       (State &s);
int                   item_count     (State &s);                     // always the source's
QList<Types::Song>    items          (State &s);                     // projected order
QPersistentModelIndex find_by_source (State &s, const QUrl &source); // projected index
bool pointed_to (State &s, const QPersistentModelIndex &projected,
                 const std::function<bool(const Types::Song &)> &callback);

// --- writers
void clear        (State &s);                                      // the view follows implicitly
void batch_append (State &s, QList<Types::Song> songs);            // lands at the end of both lists
void spawn        (State &s, QList<Types::Song> songs);            // append, then reshuffle
bool move_item    (State &s, const QPersistentModelIndex &src, const QPersistentModelIndex &dest);
void batch_remove (State &s, const QList<QPersistentModelIndex> &source_indices); // view follows implicitly

// `lead` is an index of the SOURCE; it leads the new shuffled order. Ignored when disabling.
void set_shuffled (State &s, bool enabled, const QPersistentModelIndex &lead);

}
