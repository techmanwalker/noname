#include "abstractmediasequence.hpp"
#include "audioengine.hpp"
#include "playlistsequence.hpp"
#include "songfactory.hpp"
#include "playqueue.hpp"

#include <QLoggingCategory>

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
    connect (&playing, &audio_engine::track_changed,
            this, &PlayQueue::handle_track_changed);
    
    connect (this, &PlayQueue::countChanged,
            this, &PlayQueue::preload_next_track_whenever_possible);

    connect(&playing, &audio_engine::queued_tracks_finished,
            this, &PlayQueue::handle_queued_tracks_finished);
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

void PlayQueue::clear () { 

    PlaylistSequence::clear();

    playing.stop();
    playing.unload();

    return;
}

QFuture<void> 
PlayQueue::batch_append (const QList<QUrl> &sources)
{ 
    return AbstractMediaSequence::batch_append(sources, chosen_cover_provider);
}

void
PlayQueue::respawn_queue(const QList<Types::Song> &new_queue)
{
    PlaylistSequence::respawn_list(new_queue);

    if (itemCount() > 0 && !playhead().isValid()) switch_to(index(0));
}

void
PlayQueue::respawn_queue(const PlaylistSequence &new_queue)
{
    // this version is synchronous
    PlaylistSequence::respawn_list(new_queue.items());

    if (itemCount() > 0 && !playhead().isValid()) switch_to(index(0));
}

void
PlayQueue::respawn_queue (const QStringList &sources)
{
    clear();

    QList<QUrl> uri_sources;
    uri_sources.reserve(sources.size());

    for (const QString &source : sources) {
        uri_sources.emplace_back(QUrl::fromLocalFile(source));
    }

    AbstractMediaSequence::batch_append(uri_sources, chosen_cover_provider);
}

QPersistentModelIndex
PlayQueue::playhead ()
{
    return AbstractMediaSequence::find(&Types::Song::source, playing.current_track().source);
}

void
PlayQueue::switch_to(const Types::Song &song, bool play_afterwards)
{
    const QPersistentModelIndex prolly_in_queue = AbstractMediaSequence::find(&Types::Song::source, song.source);

    /* clear the whole queue and create a new one with only
        this song como youtube music */
    PlaylistSequence replacement (QList<Types::Song> {song});
    respawn_queue(replacement);

    playing.load(items()[0]);

    if (play_afterwards) {
        playing.play();
    }
}

bool
PlayQueue::switch_to(const QPersistentModelIndex &song, bool play_afterwards)
{
    // true only means that the current song was successfully loaded

    // load the current song
    if (
        !song.isValid()
    ||   song.model() != this // not from queue
    ) return false;

    auto song_opt = pointed_to(song);
    if (!song_opt.has_value()) return false;

    Types::Any &song_item = song_opt.value().get();

    if (!std::holds_alternative<Types::Song>(song_item)) return false;

    playing.load(std::get<Types::Song>(song_item));

    if (play_afterwards) {
        playing.play();
    }


    return true;
}

void
PlayQueue::switch_to (const QUrl &source)
{
    song_factory::extract(source, chosen_cover_provider, {}).then(
        this,
        [this](Types::Song song) {
            if (song.is_valid()) {
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
    // play right after switching
    switch_to(index_next_to(playhead()), true);
}

void
PlayQueue::prev ()
{
    int last_index = itemCount() - 1;
    if (last_index < 0) return; // empty queue

    // cycle
    if (!playhead().isValid() || playhead().row() == 0) {
        switch_to(index(last_index, 0));
        return;
    }

    // play right after switching
    switch_to(index(playhead().row() - 1, 0), true);
}

void
PlayQueue::preload_next_track_whenever_possible ()
{

    // preload the next song
    QPersistentModelIndex next_song_idx = index_next_to(playhead());

    if (!next_song_idx.isValid()) playing.undo_prepare_next_track(); // avoid accidental replay

    auto song_opt = pointed_to(next_song_idx);
    if (!song_opt.has_value()) return;

    Types::Any &any_item = song_opt.value().get();

    if (!std::holds_alternative<Types::Song>(any_item)) return;

    Types::Song &song_item = std::get<Types::Song>(any_item);

    // don't preload again an already preloaded song
    if (playing.next_track_prepared().source == song_item.source) return;
    
    playing.prepare_next_track(song_item);

    qCDebug (l_mediasequences) << "Next song was successfully preloaded to play next.";
}

void
PlayQueue::handle_queued_tracks_finished()
{
    qCDebug (l_mediasequences) << "The chain of preloaded songs has finished.";

    // if ever there is a slipoff on not preloading the next song, that will be played anyway
    if (!switch_to(index_next_to(playhead()), true)) {
        playing.stop();
    }
}

void
PlayQueue::handle_track_changed ()
{
    emit track_changed();

    preload_next_track_whenever_possible();
}