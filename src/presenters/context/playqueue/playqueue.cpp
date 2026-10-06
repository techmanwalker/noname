#include "abstractmediasequence.hpp"
#include "audioengine-in.hpp"

#include "playqueue.hpp"
#include "playlistsequence.hpp"
#include "shuffleproxy.hpp"
#include "songfactory.hpp"

#include <QLoggingCategory>

#include <memory>
#include <qabstractitemmodel.h>

class PlayQueueLIPrivate 
{
public: 
    PlaylistSequence sequence;
    ShuffleProxy     order;
    std::shared_ptr<audio_engine> playing;

    PlayQueueLIPrivate(
        std::shared_ptr<audio_engine> controller
    ) :
        playing(controller)
    {}
};

PlayQueueLI::PlayQueueLI (
    QObject *parent,
    std::shared_ptr<audio_engine> controller
)
    : QIdentityProxyModel(parent),
      m_d(std::make_unique<PlayQueueLIPrivate> (controller))
{
    // Binds the hidden sequence so QIdentityProxyModel automatically forwards model data
    m_d->order.setSourceModel(&m_d->sequence);
    setSourceModel(&m_d->order);
    
    connect (&m_d->sequence, &PlaylistSequence::countChanged,
            this, &PlayQueueLI::countChanged);
            
    connect (this, &PlayQueueLI::countChanged,
            this, &PlayQueueLI::preload_next_track_whenever_possible);
}

PlayQueueLI::~PlayQueueLI() = default;


QList<Types::Song>
PlayQueueLI::items () const
{ 
    return m_d->sequence.items(); 
}

int
PlayQueueLI::itemCount () const
{
    return m_d->sequence.itemCount();
}

void
PlayQueueLI::clear ()
{ 
    m_d->sequence.clear();
    m_d->playing->stop();
    m_d->playing->unload();
}

QFuture<void>
PlayQueueLI::batch_append (const QList<QUrl> &sources)
{
    return song_factory::batch_extract(sources, {}).then(this, [this] (QList<Types::Song> to_append) {
        m_d->sequence.batch_append(std::move(to_append));
    });
}

void
PlayQueueLI::respawn_queue(const QList<Types::Song> &new_queue)
{
    m_d->sequence.respawn_list(new_queue);
    if (rowCount() > 0 && !playhead().isValid()) switch_to(index(0, 0));
}

void
PlayQueueLI::respawn_queue (const QStringList &sources)
{
    clear();

    QList<QUrl> uri_sources;
    uri_sources.reserve(sources.size());

    for (const QString &source : sources) {
        uri_sources.emplace_back(QUrl::fromLocalFile(source));
    }

    batch_append(uri_sources);
}

QPersistentModelIndex
PlayQueueLI::playhead ()
{
    QPersistentModelIndex src_idx = m_d->sequence.find(&Types::Song::source, m_d->playing->current_track().source);
    if (!src_idx.isValid()) return {};
    
    // Map the internal sequence index to the proxy index facing QML
    return from_sequence(src_idx);
}

void
PlayQueueLI::switch_to(const Types::Song &song, bool play_afterwards)
{
    m_d->sequence.respawn_list({song});
    m_d->playing->load(items()[0]);

    if (play_afterwards) {
        m_d->playing->play();
    }
}

bool
PlayQueueLI::switch_to(const QPersistentModelIndex &song, bool play_afterwards)
{
    if (!song.isValid() || song.model() != this) return false;

    if (song == playhead()) {
        if (play_afterwards) m_d->playing->play();
        return true; 
    }

    // Map the external proxy index down to the hidden sequence index
    auto song_opt = m_d->sequence.pointed_to(QPersistentModelIndex(to_sequence(song)));
    if (!song_opt.has_value()) return false;

    Types::Any &song_item = song_opt.value().get();
    if (!std::holds_alternative<Types::Song>(song_item)) return false;

    m_d->playing->load(std::get<Types::Song>(song_item));

    if (play_afterwards) {
        m_d->playing->play();
    }

    return true;
}

void
PlayQueueLI::switch_to (const QUrl &source){
    song_factory::extract(source, {}).then(
        this,
        [this](Types::Song song) {
            if (song.is_valid()) {
                switch_to(song, true);
            }
        }
    );
}

void
PlayQueueLI::qml_switch_to(const QModelIndex &song)
{
    switch_to(QPersistentModelIndex(song), true);
}

void
PlayQueueLI::next ()
{
    QPersistentModelIndex current = playhead();
    if (!current.isValid()) return;
    
    const QPersistentModelIndex upcoming = successor_of(current);
    if (upcoming.isValid()) switch_to(upcoming, true);
}

void
PlayQueueLI::prev ()
{
    int last_index = rowCount() - 1;
    if (last_index < 0) return; 

    if (!playhead().isValid() || playhead().row() == 0) {
        switch_to(index(last_index, 0));
        return;
    }

    switch_to(index(playhead().row() - 1, 0), true);
}

void
PlayQueueLI::preload_next_track_whenever_possible ()
{
    const QPersistentModelIndex upcoming = successor_of(playhead());

    if (!upcoming.isValid()) m_d->playing->undo_prepare_next_track();

    auto song_opt = m_d->sequence.pointed_to(QPersistentModelIndex(to_sequence(upcoming)));
    if (!song_opt.has_value()) return;

    Types::Any &any_item = song_opt.value().get();
    if (!std::holds_alternative<Types::Song>(any_item)) return;

    Types::Song &song_item = std::get<Types::Song>(any_item);
    if (m_d->playing->next_track_prepared().source == song_item.source) return;
    
    m_d->playing->prepare_next_track(song_item);
    qCDebug (l_mediasequences) << "Next song was successfully preloaded to play next.";
}

void
PlayQueueLI::handle_queued_tracks_finished()
{
    qCDebug (l_mediasequences) << "The chain of preloaded songs has finished.";
    
    if (!switch_to(successor_of(playhead()), true)) {
        m_d->playing->pause(); 
    }
}

QPersistentModelIndex
PlayQueueLI::find_by_source(const QString &needle)
{
    return from_sequence(m_d->sequence.find<Types::Song, QUrl>(&Types::Song::source, needle));
}

void
PlayQueueLI::handle_track_changed ()
{
    emit trackChanged();
    preload_next_track_whenever_possible();
}

QModelIndex
PlayQueueLI::to_sequence (const QModelIndex &mine) const
{
    return m_d->order.mapToSource(mapToSource(mine));
}

QPersistentModelIndex
PlayQueueLI::from_sequence (const QModelIndex &seq) const
{
    return QPersistentModelIndex(mapFromSource(m_d->order.mapFromSource(seq)));
}

QPersistentModelIndex
PlayQueueLI::successor_of (const QPersistentModelIndex &mine) const
{
    if (!mine.isValid() || mine.row() + 1 >= rowCount()) return {};
    return QPersistentModelIndex(index(mine.row() + 1, 0));
}

bool
PlayQueueLI::shuffled () const
{
    return m_d->order.shuffled();
}

void
PlayQueueLI::set_shuffle (bool enabled)
{
    if (enabled == shuffled()) return;

    // The playing song becomes the head of the shuffled order, so "next" is a
    // random upcoming track instead of the song drifting to a random slot.
    int pinned = -1;
    if (enabled) {
        const QModelIndex now = to_sequence(playhead());
        if (now.isValid()) pinned = now.row();
    }

    m_d->order.setShuffled(enabled, pinned);

    emit shuffleChanged();
    emit trackChanged(); // same track, different row: QML must re-read PlayQueue.playhead
    preload_next_track_whenever_possible(); // the upcoming track changed
}