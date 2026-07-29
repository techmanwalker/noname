#include "abstractmediasequence.hpp"
#include "playlistsequence.hpp"
#include "songfactory.hpp"
#include "playqueue.hpp"

// Meyers singleton implementation
PlayQueue &
PlayQueue::instance()
{
    static PlayQueue s_instance;
    return s_instance;
}

PlayQueue::PlayQueue(QObject *parent)
    : PlaylistSequence(parent)
{
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



// --- The Play queue ---

QList<Types::Song> PlayQueue::items () const { return AbstractMediaSequence::items<Types::Song>() ; }

QFuture<void> 
PlayQueue::batch_append (const QList<QUrl> &sources)
{ 
    return AbstractMediaSequence::batch_append(sources, chosen_cover_provider);
}

void
PlayQueue::respawn_queue(const QList<Types::Song> &new_queue)
{
    PlaylistSequence::respawn_list(new_queue);

    if (itemCount() > 0 && !m_playhead.isValid()) switch_to(index(0));
}

void
PlayQueue::respawn_queue(const PlaylistSequence &new_queue)
{
    // this version is synchronous
    PlaylistSequence::respawn_list(new_queue.items());

    if (itemCount() > 0 && !m_playhead.isValid()) switch_to(index(0));
}

QModelIndex
PlayQueue::playhead () const
{
    return m_playhead;
}

void
PlayQueue::switch_to(const Types::Song &song, bool play_afterwards)
{
    const QPersistentModelIndex prolly_in_queue = AbstractMediaSequence::find(&Types::Song::source, song.source);

    // not in queue
    if (!prolly_in_queue.isValid()) {
        /* clear the whole queue and create a new one with only
           this song como youtube music */
        PlaylistSequence replacement (QList<Types::Song> {song});
        respawn_queue(replacement);

        m_playhead = index(0);

        emit playheadChanged(play_afterwards);
        return;
    }

    m_playhead = prolly_in_queue;

    emit playheadChanged(play_afterwards);
    return;
}

void
PlayQueue::switch_to(const QPersistentModelIndex &song, bool play_afterwards)
{
    if (
        !song.isValid()
    ||  m_playhead == song   // we are currently in that index
    ||  song.model() != this // not from queue
    ) return;

    m_playhead = song;

    emit playheadChanged(play_afterwards);
}

void
PlayQueue::switch_to (const QModelIndex &song, bool play_afterwards)
{
    if (
        !song.isValid()
    ||  m_playhead == song   // we are currently in that index
    ||  song.model() != this // not from queue
    ) return;

    // QPersistentModelIndex accepts QModelIndex in its = operator =)
    m_playhead = song;

    emit playheadChanged(play_afterwards);
}

void
PlayQueue::switch_to (const QUrl &source)
{
    song_factory::extract(source, chosen_cover_provider).then(
        this,
        [this](Types::Song song) {
            if (!song.source.isEmpty()) {
                switch_to(song, true);
            }
        }
    );
}

void
PlayQueue::qml_switch_to(const QModelIndex &song)
{
    // play after switching
    switch_to(song, true);
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

    // play right after switching
    switch_to(index(m_playhead.row() + 1), true);
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

    // play right after switching
    switch_to(index(m_playhead.row() - 1, 0), true);
}
