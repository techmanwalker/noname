#include "playqueue.hpp"
#include "abstractmediasequence.hpp"
#include "playlistsequence.hpp"
#include <QQmlEngine>
#include <QAbstractItemModel>
#include <QFuture>
#include <QList>

PlayQueue::PlayQueue(QObject *parent)
    : PlaylistSequence(parent)
{
}

int PlayQueue::itemCount () const { return AbstractMediaSequence::itemCount(); }
QFuture<void> PlayQueue::batch_append(const QList<QUrl> &sources) { return PlaylistSequence::batch_append(sources); }

QPersistentModelIndex
PlayQueue::find (const Types::Song &needle) const
{
    // explicit rvalue
    return AbstractMediaSequence::find(Types::Any{needle});
}

// Meyers singleton implementation
PlayQueue &
PlayQueue::instance()
{
    static PlayQueue s_instance;
    return s_instance;
}

// factory for the qml engine
PlayQueue *
PlayQueue::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    
    PlayQueue *inst = &instance();

    // avoid QML GC to try to free object memory
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}

void
PlayQueue::respawn_queue(QList<QUrl> new_queue)
{
    clear();

    // this version is async
    batch_append(new_queue).then([this]() {
            if (itemCount() > 0 && !m_playhead.isValid()) switch_to(index(0));
        }
    );
}

void
PlayQueue::respawn_queue(PlaylistSequence &new_queue)
{
    clear();

    // this version is synchronous
    PlaylistSequence::batch_append(new_queue.items<Types::Song>());

    if (itemCount() > 0 && !m_playhead.isValid()) switch_to(index(0));
}

void
PlayQueue::switch_to(const Types::Song &song)
{
    const QPersistentModelIndex prolly_in_queue = find(song);

    // not in queue
    if (!prolly_in_queue.isValid()) {
        // clear the whole queue and create a new one with only
        // this song como youtube music
        PlaylistSequence replacement (QList<Types::Song> {song});
        respawn_queue(replacement);

        m_playhead = index(0);

        emit playheadChanged();
        return;
    }

    m_playhead = prolly_in_queue;

    emit playheadChanged();
    return;
}

void
PlayQueue::switch_to(const QPersistentModelIndex &song)
{
    if (
        !song.isValid()
    ||  m_playhead == song   // we are currently in that index
    ||  song.model() != this // not from queue
    ) return;

    m_playhead = song;

    emit playheadChanged();
}

void
PlayQueue::switch_to (QModelIndex song)
{
    if (
        !song.isValid()
    ||  m_playhead == song   // we are currently in that index
    ||  song.model() != this // not from queue
    ) return;

    // QPersistentModelIndex accepts QModelIndex in its = operator =)
    m_playhead = song;

    emit playheadChanged();
}

void
PlayQueue::next ()
{
    if (itemCount() == 0) return;

    // cycle
    if (!m_playhead.isValid() || m_playhead.row() == itemCount() - 1) {
        switch_to(index(0));
        return;
    }

    switch_to(index(m_playhead.row() + 1));
}

void
PlayQueue::prev ()
{
    int last_index = itemCount() - 1;
    if (last_index < 0) return; // empty queue

    // cycle
    if (!m_playhead.isValid() || m_playhead.row() == 0) {
        switch_to(index(last_index, 0));
        return;
    }

    switch_to(index(m_playhead.row() - 1, 0));
}