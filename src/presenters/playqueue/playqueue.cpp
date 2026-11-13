#include "abstractmediasequence.hpp"
#include "virtual/audioengine.hpp"

#include "playqueue.hpp"
#include "playqueuesequence.hpp"
#include "songfactory.hpp"
#include "virtual/playlistslist.hpp"

#include <QLoggingCategory>
#include <QtAssert>

#include <memory>
#include <optional>

Q_LOGGING_CATEGORY(l_playqueue, "playqueue")

class PlayQueueLIPrivate 
{
public: 
    sequence::State                state;
    std::shared_ptr<audio_engine>  ae;
    std::shared_ptr<PlaylistsList> pl;

    PlayQueueLIPrivate(
        std::shared_ptr<audio_engine> audioengine,
        std::shared_ptr<PlaylistsList> playlistslist
    ) :
        ae(std::move(audioengine)),
        pl(std::move(playlistslist))
    {}
};

PlayQueueLI::PlayQueueLI (
    QObject *parent,
    std::shared_ptr<audio_engine> audioengine,
    std::shared_ptr<PlaylistsList> playlistslist
)
    : QIdentityProxyModel(parent),
      m_d(std::make_unique<PlayQueueLIPrivate> (audioengine, playlistslist))
{
    // There are two lists behind this queue (the source and its shuffled view) and only the
    // sequence namespace may touch them. We present whatever it projects.
    setSourceModel(sequence::projection(m_d->state));

    sequence::on_count_changed(m_d->state, this, [this] { emit count_changed(); });

    connect (this, &PlayQueueLI::count_changed,
            this, &PlayQueueLI::preload_next_track_whenever_possible);

    connect (this, &PlayQueueLI::count_changed,
            this, &PlayQueueLI::handle_zero_item_count);

    connect (this, &PlayQueueLI::track_changed,
            this, &PlayQueueLI::preload_next_track_whenever_possible);
}

PlayQueueLI::~PlayQueueLI() = default;


QList<Types::Song>
PlayQueueLI::items () const
{ 
    return sequence::items(m_d->state); // projected order
}

QList<QUrl>
PlayQueueLI::sources() const
{
    QList<QUrl> uris;

    for (Types::Song item : items()) {
        uris.append(std::move(item.source));
    }

    return uris;
}

int
PlayQueueLI::item_count () const
{
    return sequence::item_count(m_d->state);
}

void
PlayQueueLI::clear ()
{ 
    sequence::clear(m_d->state);
}

QFuture<void>
PlayQueueLI::batch_append (const QList<QUrl> &sources)
{
    return song_factory::batch_extract(sources, {}).then(this, [this] (QList<Types::Song> to_append) {
        sequence::batch_append(m_d->state, std::move(to_append));
    });

    // append is done at the end of both projections, playhead remains unchanged
}

QFuture<void>
PlayQueueLI::spawn_queue(const QStringList &sources)
{
    QList<QUrl> uri_sources;
    uri_sources.reserve(sources.size());

    for (const QString &source : sources) {
        uri_sources.emplace_back(QUrl::fromLocalFile(source));
    }

    return song_factory::batch_extract(uri_sources, {}).then(this, [this] (QList<Types::Song> to_spawn) {
        sequence::spawn(m_d->state, std::move(to_spawn));

        // the reshuffle emits nothing the queue listens to: QML must re-read the playhead
        // and the upcoming track changed
        emit track_changed();
    });
}

bool
PlayQueueLI::move_item(const QPersistentModelIndex &src, const QPersistentModelIndex &dest)
{
    if (src.model() != this || dest.model() != this) return false;

    if (!sequence::move_item(m_d->state,
                             QPersistentModelIndex(mapToSource(src)),
                             QPersistentModelIndex(mapToSource(dest)))) return false;

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

    sequence::spawn(m_d->state, std::move(songs));
    emit track_changed();
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

    sequence::batch_remove(m_d->state, mapped_indices);
}

void
PlayQueueLI::handle_zero_item_count ()
{
    if (item_count() == 0) {
        m_d->ae->stop();
        m_d->ae->unload();

        invalidate_playhead_();
    }
}

QPersistentModelIndex
PlayQueueLI::playhead () const
{
    if (!m_playhead.isValid()) return {};

#ifndef QT_NO_DEBUG
    QUrl at_head;
    pointed_to(m_playhead, [&at_head](const Types::Song &song) { at_head = song.source; return true; });

    QUrl actually_playing = m_d->ae->current_track_source();

    Q_ASSERT_X(at_head == actually_playing, "PlayQueueLI::playhead",
        QStringLiteral("The track at the projected playhead is not the track the audio engine is playing. Expected %1, got %2 and playhead points to row %3.")
            .arg(actually_playing.toString(), at_head.toString(), QString::number(m_playhead.row())).toUtf8().constData());
#endif

    return m_playhead;
}

void
PlayQueueLI::set_playhead_(const QPersistentModelIndex &to)
{
    // bookkeeping only, never touches the engine's tape
    m_playhead = to;
    preload_next_track_whenever_possible();
}

void
PlayQueueLI::move_playhead_(const QPersistentModelIndex &to)
{
    if (!to.isValid()) return;

    Types::Song at_playhead {};

    bool executed = pointed_to(to, [&at_playhead](const Types::Song &prolly_requested) {
        at_playhead = prolly_requested;
        return true;
    });

    Q_ASSERT_X(
        executed && at_playhead.is_valid(),
        "PlayQueueLI::move_playhead_",
        "Attempted to move playhead, but the index pointed to an invalid position."
    );

    m_d->ae->load(at_playhead);
    set_playhead_(to);

#ifndef QT_NO_DEBUG
    // fresh confirmation
    (void) playhead(); // sentinel: asserts if the playhead landed on the wrong spot
#endif

    emit track_changed();
    return;
}

void
PlayQueueLI::invalidate_playhead_()
{
    m_playhead = QPersistentModelIndex ();

    emit track_changed();
}

void
PlayQueueLI::switch_to(const Types::Song &song, bool play_afterwards)
{
    sequence::clear(m_d->state);
    sequence::batch_append(m_d->state, QList<Types::Song>{song});

    move_playhead_(index(0, 0));

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

    return pointed_to(song, [this, song, play_afterwards](const Types::Song &prolly_song) -> bool {
        move_playhead_(song);

        if (play_afterwards) {
            m_d->ae->play();
        }

        return true;
    });
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

    pointed_to(upcoming, [this](const Types::Song &song_item) {
        if (m_d->ae->next_track_prepared_source() == song_item.source) return true;

        m_d->ae->prepare_next_track(song_item);
        qCDebug (l_mediasequences) << "Next song was successfully preloaded to play next.";
        return true;
    });
}

void
PlayQueueLI::handle_tape_ran_out()
{
    qCDebug (l_mediasequences) << "The chain of preloaded songs has finished.";
    
    if (!switch_to(successor_of(playhead()), true)) {
        m_d->ae->pause(); 
    }
}

QPersistentModelIndex
PlayQueueLI::find_by_source(const QUrl &needle)
{
    return QPersistentModelIndex(mapFromSource(sequence::find_by_source(m_d->state, needle)));
}

std::optional<Types::Song>
PlayQueueLI::query_metadata_for_source(const QUrl &needle) const
{
    std::optional<Types::Song> result = std::nullopt;

    const bool success = sequence::pointed_to(
        m_d->state,
        sequence::find_by_source(m_d->state, needle),
        [&result](const Types::Song &prolly_song) {
            result = prolly_song;
            return true;
        }
    );

    if (!success) return std::nullopt;

    return result;
}

QModelIndex
PlayQueueLI::to_sequence (const QModelIndex &mine) const
{
    return sequence::to_source(m_d->state, mapToSource(mine));
}

QPersistentModelIndex
PlayQueueLI::successor_of (const QPersistentModelIndex &mine) const
{
    // the successor of an invalid playhead is zero
    if (!m_playhead.isValid()) return QPersistentModelIndex (index (0, 0));

    return QPersistentModelIndex(mapFromSource(sequence::successor_of(m_d->state, mapToSource(mine))));
}

bool
PlayQueueLI::shuffled () const
{
    return sequence::shuffled(m_d->state);
}

void
PlayQueueLI::set_shuffle (bool enabled)
{
    if (enabled == shuffled()) return;

    // The playing song leads the shuffled order, so "next" is a random upcoming track
    // instead of the song drifting to a random slot. Its index in the source is all
    // the proxy needs; it is only meaningful when enabling.
    const QPersistentModelIndex lead = enabled ? QPersistentModelIndex(to_sequence(playhead()))
                                               : QPersistentModelIndex();

    sequence::set_shuffled(m_d->state, enabled, lead);

    emit shuffle_changed();
    emit track_changed(); // same track, different row: QML must re-read PlayQueue.playhead
    preload_next_track_whenever_possible(); // the upcoming track changed
}

bool
PlayQueueLI::pointed_to (const QPersistentModelIndex &idx, std::function<bool(const Types::Song &)> callback) const
{
    return sequence::pointed_to(m_d->state, QPersistentModelIndex(mapToSource(idx)), callback);
}

QFuture<bool>
PlayQueueLI::save_as_playlist_to_disk (const QString &name)
{
    return m_d->pl->save_playlist_to_disk(name, sources());
}

void
PlayQueueLI::handle_ab_boundary_crossed ()
{
    // readback on natural progression of the tape which crossed to track B

    // the engine already crossed to B, just follow it
    // do not use the asserted playhead(), the assert below exists for that
    set_playhead_(successor_of(m_playhead));

#ifndef QT_NO_DEBUG
    Q_ASSERT_X(m_playhead.isValid(), "PlayQueueLI::handle_ab_boundary_crossed",
            "The engine crossed to B, but the queue has no successor to follow it.");
    (void) playhead(); // sentinel: asserts if the playhead landed on the wrong spot
#endif

    emit track_changed();
}
