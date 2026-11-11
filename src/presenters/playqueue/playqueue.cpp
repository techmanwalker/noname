#include "abstractmediasequence.hpp"
#include "virtual/audioengine.hpp"

#include "playqueue.hpp"
#include "playlistsequence.hpp"
#include "shuffleproxy.hpp"
#include "songfactory.hpp"

#include <QLoggingCategory>
#include <QtAssert>

#include <memory>
#include <optional>
#include <qabstractitemmodel.h>
#include <qlist.h>

class PlayQueueLIPrivate 
{
public: 
    PlaylistSequence              sequence;
    MediaTransforms::ShuffleProxy order;
    std::shared_ptr<audio_engine> ae;

    PlayQueueLIPrivate(
        std::shared_ptr<audio_engine> audioengine
    ) :
        ae(std::move(audioengine))
    {}
};

PlayQueueLI::PlayQueueLI (
    QObject *parent,
    std::shared_ptr<audio_engine> audioengine
)
    : QIdentityProxyModel(parent),
      m_d(std::make_unique<PlayQueueLIPrivate> (audioengine))
{
    // Do not forget that PlayQueue is actually two switching models and one is not
    // a plain PlaylistSequence, so be careful with signal handling and data writes
    // that affect either model.


    // Binds the hidden sequence so QIdentityProxyModel automatically forwards model data
    m_d->order.setSourceModel(&m_d->sequence);
    setSourceModel(&m_d->order);
    
    connect (&m_d->sequence, &PlaylistSequence::count_changed,
            this, &PlayQueueLI::count_changed);
            
    connect (this, &PlayQueueLI::count_changed,
            this, &PlayQueueLI::preload_next_track_whenever_possible);
}

PlayQueueLI::~PlayQueueLI() = default;


QList<Types::Song>
PlayQueueLI::items () const
{ 
    return m_d->sequence.items(); 
}

int
PlayQueueLI::item_count () const
{
    return m_d->sequence.rowCount();
}

void
PlayQueueLI::clear ()
{ 
    m_d->sequence.clear();
    m_d->ae->stop();
    m_d->ae->unload();
}

QFuture<void>
PlayQueueLI::batch_append (const QList<QUrl> &sources)
{
    return song_factory::batch_extract(sources, {}).then(this, [this] (QList<Types::Song> to_append) {
        m_d->sequence.batch_append(std::move(to_append));
    });
}

QFuture<void>
PlayQueueLI::spawn_queue(const QStringList &sources)
{
    QList<QUrl> uri_sources;
    uri_sources.reserve(sources.size());

    for (const QString &source : sources) {
        uri_sources.emplace_back(QUrl::fromLocalFile(source));
    }

    return batch_append(uri_sources);
}

bool
PlayQueueLI::move_item(const QPersistentModelIndex &src, const QPersistentModelIndex &dest)
{
    if (!m_d->sequence.move_item(to_sequence(src), to_sequence(dest))) return false;

    // same track, maybe a different row: QML only re-reads PlayQueue.playhead on track_changed,
    // and the upcoming track may have changed
    emit track_changed();
    preload_next_track_whenever_possible();
    return true;
}

QFuture<void>
PlayQueueLI::respawn_queue (const QStringList &sources)
{
    clear();

    return spawn_queue(sources);
}

void
PlayQueueLI::respawn_queue(QList<Types::Song> songs)
{
    clear();

    m_d->sequence.batch_append(std::move(songs));
}

void
PlayQueueLI::batch_remove(const QList<QPersistentModelIndex> &indices)
{
    QList<QPersistentModelIndex> mapped_indices;
    mapped_indices.reserve(indices.size());

    for (const QPersistentModelIndex &idx : indices) {
        if (!idx.isValid()) continue;
        const QModelIndex seq = to_sequence(idx);
        if (!seq.isValid()) continue;
        mapped_indices.append(QPersistentModelIndex(seq));
    }

    m_d->sequence.batch_remove(mapped_indices);
}

QPersistentModelIndex
PlayQueueLI::playhead ()
{
    QPersistentModelIndex src_idx = m_d->sequence.find(&Types::Song::source, m_d->ae->current_track_source());
    if (!src_idx.isValid()) return {};
    
    // Map the internal sequence index to the proxy index facing QML
    return from_sequence(src_idx);
}

void
PlayQueueLI::switch_to(const Types::Song &song, bool play_afterwards)
{
    m_d->sequence.respawn_list(std::array<Types::Song, 1>{song});
    m_d->ae->load(items()[0]);

    if (play_afterwards) {
        m_d->ae->play();
    }
}

bool
PlayQueueLI::switch_to(const QPersistentModelIndex &song, bool play_afterwards)
{
    if (!song.isValid() || song.model() != this) return false;

    if (song == playhead()) {
        if (play_afterwards) m_d->ae->play();
        return true; 
    }

    // Map the external proxy index down to the hidden sequence index
    return m_d->sequence.pointed_to<const Types::Song &>(
        QPersistentModelIndex(to_sequence(song)), 
        [this, play_afterwards](const Types::Song &prolly_song) -> bool {
            m_d->ae->load(prolly_song);

            if (play_afterwards) {
                m_d->ae->play();
            }

            return true;
        }
    );
}

bool
PlayQueueLI::switch_to(size_t idx, bool play_afterwards)
{
    // rowCount() is a non-negative int, so once idx is below it the cast to int is lossless
    if (idx >= static_cast<size_t>(rowCount())) return false;

    return switch_to(index(static_cast<int>(idx), 0), play_afterwards);
}

void
PlayQueueLI::switch_to (const QUrl &source){
    song_factory::extract(source, {}).then(
        this,
        [this](Types::Song song) {
            if (song.is_valid()) {
                switch_to(std::move(song), true);
            }
        }
    );
}

void
PlayQueueLI::qml_switch_to(const QModelIndex &song)
{
    switch_to(QPersistentModelIndex(song), true);
}

bool
PlayQueueLI::next ()
{
    QPersistentModelIndex current = playhead();
    if (!current.isValid()) {
        // switch to first track
        return switch_to(0, true);
    };
    
    const QPersistentModelIndex upcoming = successor_of(current);
    if (upcoming.isValid()) {
        return switch_to(upcoming, true);
    } else {
        // go back to first track
        return switch_to(index(0, 0));
    }
}

bool
PlayQueueLI::prev (bool play_afterwards)
{
    int last_index = rowCount() - 1;
    if (last_index < 0) return false; 

    if (!playhead().isValid() || playhead().row() == 0) {
        return switch_to(index(last_index, 0));
    }

    return switch_to(index(playhead().row() - 1, 0), play_afterwards);
}

bool
PlayQueueLI::prev_at_end(quint64 stepback_ms, bool play_afterwards)
{   
    // step back
    QUrl prolly_not_the_same_track = m_d->ae->current_track_source();
    prev();

    auto playhead_item_ref = query_metadata_for_source(m_d->ae->current_track_source());

    if (!playhead_item_ref.has_value()) {
        qCDebug(l_mediasequences) << "Current playing track not found on the queue.";
        return false;
    }

    if (prolly_not_the_same_track == playhead_item_ref->source) {
        // still same track

        // seek back to beginning
        m_d->ae->seek_to(0);
        if (play_afterwards) m_d->ae->play();

        // never truly switched
        return false;
    }

    Q_ASSERT_X(m_d->ae->current_track_source() == playhead_item_ref.value().source,
        "PlayQueueLI::prev_at_end",
        "The supposed playhead track reference is not the current track according to audio_engine."
    );

    quint64 track_duration = playhead_item_ref.value().duration;

    if (stepback_ms < track_duration) {
        m_d->ae->seek_to(track_duration - stepback_ms);
    } else [[unlikely]] {
        m_d->ae->seek_to(0);
    }

    if (play_afterwards) m_d->ae->play();

    return true;
}

void
PlayQueueLI::preload_next_track_whenever_possible ()
{
    const QPersistentModelIndex upcoming = successor_of(playhead());

    if (!upcoming.isValid()) m_d->ae->undo_prepare_next_track();

    m_d->sequence.pointed_to<const Types::Song &>(
        QPersistentModelIndex(to_sequence(upcoming)), 
        [this](const Types::Song &prolly_song) {
            const Types::Song &song_item = prolly_song;
            if (m_d->ae->next_track_prepared_source() == song_item.source) return;

            m_d->ae->prepare_next_track(song_item);
            qCDebug (l_mediasequences) << "Next song was successfully preloaded to play next.";
        }
    );
}

void
PlayQueueLI::handle_queued_tracks_finished()
{
    qCDebug (l_mediasequences) << "The chain of preloaded songs has finished.";
    
    if (!switch_to(successor_of(playhead()), true)) {
        m_d->ae->pause(); 
    }
}

QPersistentModelIndex
PlayQueueLI::find_by_source(const QUrl &needle)
{
    return from_sequence(m_d->sequence.find<Types::Song, QUrl>(&Types::Song::source, needle));
}

std::optional<Types::Song>
PlayQueueLI::query_metadata_for_source(const QUrl &needle) const
{
    std::optional<Types::Song> result = std::nullopt;

    bool success = m_d->sequence.pointed_to<const Types::Song &> (
        m_d->sequence.find<Types::Song, QUrl>(&Types::Song::source, needle), 
        [&result](const Types::Song &prolly_song) -> void {
            result = prolly_song;
        }
    );

    if (!success) return std::nullopt;

    return result;
}

void
PlayQueueLI::handle_track_changed ()
{
    emit track_changed();
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

    emit shuffle_changed();
    emit track_changed(); // same track, different row: QML must re-read PlayQueue.playhead
    preload_next_track_whenever_possible(); // the upcoming track changed
}

bool
PlayQueueLI::pointed_to (const QPersistentModelIndex &idx, std::function<bool(const Types::Song &)> callback)
{
    return pointed_to_t<Types::Song>(idx, callback);
}

PlaylistSequence &
PlayQueueLI::_sequence() const
{
    return m_d->sequence;
}
