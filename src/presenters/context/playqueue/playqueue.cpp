#include "audioengine-in.hpp"

#include "playqueue.hpp"
#include "playlistsequence.hpp"
#include "coverprovider.hpp"
#include "songfactory.hpp"

#include <QLoggingCategory>
#include <QJSEngine>
#include <QQmlEngine>
#include <memory>

class PlayQueueLIPrivate {
public: 
    PlaylistSequence sequence;
    std::shared_ptr<audio_engine> playing;
    std::shared_ptr<covers::live::cover_provider> chosen_cover_provider;

    PlayQueueLIPrivate(
        std::shared_ptr<covers::live::cover_provider> cover_provider,
        std::shared_ptr<audio_engine> controller
    ) :
        chosen_cover_provider(cover_provider),
        playing(controller)
    {}
};

PlayQueueLI::PlayQueueLI (
    QObject *parent,
    std::shared_ptr<covers::live::cover_provider> cover_provider,
    std::shared_ptr<audio_engine> controller
)
    : QIdentityProxyModel(parent),
      m_d(std::make_unique<PlayQueueLIPrivate>(cover_provider, controller))
{
    // Binds the hidden sequence so QIdentityProxyModel automatically forwards model data
    setSourceModel(&m_d->sequence);
    
    connect (&m_d->sequence, &PlaylistSequence::countChanged,
            this, &PlayQueueLI::countChanged);
            
    connect (this, &PlayQueueLI::countChanged,
            this, &PlayQueueLI::preload_next_track_whenever_possible);
}

PlayQueueLI::~PlayQueueLI() = default;


QList<Types::Song> PlayQueueLI::items () const { 
    return m_d->sequence.items(); 
}

int PlayQueueLI::itemCount () const {
    return m_d->sequence.itemCount();
}

void PlayQueueLI::clear () { 
    m_d->sequence.clear();
    m_d->playing->stop();
    m_d->playing->unload();
}

QFuture<void> PlayQueueLI::batch_append (const QList<QUrl> &sources) {
    return song_factory::batch_extract(sources, {}).then(this, [this] (QList<Types::Song> to_append) {
        if (m_d->chosen_cover_provider) {
            for (const Types::Song &song : to_append) {
                m_d->chosen_cover_provider->register_cover_reference(song.cover);
            }
        }
        m_d->sequence.batch_append(std::move(to_append));
    });
}

void PlayQueueLI::respawn_queue(const QList<Types::Song> &new_queue) {
    m_d->sequence.respawn_list(new_queue);
    if (rowCount() > 0 && !playhead().isValid()) switch_to(index(0, 0));
}

void PlayQueueLI::respawn_queue (const QStringList &sources) {
    clear();

    QList<QUrl> uri_sources;
    uri_sources.reserve(sources.size());

    for (const QString &source : sources) {
        uri_sources.emplace_back(QUrl::fromLocalFile(source));
    }

    batch_append(uri_sources);
}

QPersistentModelIndex PlayQueueLI::playhead () {
    QPersistentModelIndex src_idx = m_d->sequence.find(&Types::Song::source, m_d->playing->current_track().source);
    if (!src_idx.isValid()) return {};
    
    // Map the internal sequence index to the proxy index facing QML
    return QPersistentModelIndex(mapFromSource(src_idx));
}

void PlayQueueLI::switch_to(const Types::Song &song, bool play_afterwards) {
    m_d->sequence.respawn_list({song});
    m_d->playing->load(items()[0]);

    if (play_afterwards) {
        m_d->playing->play();
    }
}

bool PlayQueueLI::switch_to(const QPersistentModelIndex &song, bool play_afterwards) {
    if (!song.isValid() || song.model() != this) return false;

    if (song == playhead()) {
        if (play_afterwards) m_d->playing->play();
        return true; 
    }

    // Map the external proxy index down to the hidden sequence index
    QModelIndex src_idx = mapToSource(song);
    auto song_opt = m_d->sequence.pointed_to(src_idx);
    if (!song_opt.has_value()) return false;

    Types::Any &song_item = song_opt.value().get();
    if (!std::holds_alternative<Types::Song>(song_item)) return false;

    m_d->playing->load(std::get<Types::Song>(song_item));

    if (play_afterwards) {
        m_d->playing->play();
    }

    return true;
}

void PlayQueueLI::switch_to (const QUrl &source) {
    song_factory::extract(source, {}).then(
        this,
        [this](Types::Song song) {
            if (song.is_valid()) {
                if (m_d->chosen_cover_provider) {
                    m_d->chosen_cover_provider->register_cover_reference(song.cover);
                }
                switch_to(song, true);
            }
        }
    );
}

void PlayQueueLI::qml_switch_to(const QModelIndex &song) {
    switch_to(QPersistentModelIndex(song), true);
}

void PlayQueueLI::next () {
    QPersistentModelIndex current = playhead();
    if (!current.isValid()) return;
    
    QPersistentModelIndex src_next = m_d->sequence.index_next_to(QPersistentModelIndex(mapToSource(current)));
    if (src_next.isValid()) {
        switch_to(QPersistentModelIndex(mapFromSource(src_next)), true);
    }
}

void PlayQueueLI::prev () {
    int last_index = rowCount() - 1;
    if (last_index < 0) return; 

    if (!playhead().isValid() || playhead().row() == 0) {
        switch_to(index(last_index, 0));
        return;
    }

    switch_to(index(playhead().row() - 1, 0), true);
}

void PlayQueueLI::preload_next_track_whenever_possible () {
    QPersistentModelIndex src_next = m_d->sequence.index_next_to(QPersistentModelIndex(mapToSource(playhead())));

    if (!src_next.isValid()) m_d->playing->undo_prepare_next_track();

    auto song_opt = m_d->sequence.pointed_to(src_next);
    if (!song_opt.has_value()) return;

    Types::Any &any_item = song_opt.value().get();
    if (!std::holds_alternative<Types::Song>(any_item)) return;

    Types::Song &song_item = std::get<Types::Song>(any_item);
    if (m_d->playing->next_track_prepared().source == song_item.source) return;
    
    m_d->playing->prepare_next_track(song_item);
    qCDebug (l_mediasequences) << "Next song was successfully preloaded to play next.";
}

void PlayQueueLI::handle_queued_tracks_finished() {
    qCDebug (l_mediasequences) << "The chain of preloaded songs has finished.";
    
    QPersistentModelIndex src_next = m_d->sequence.index_next_to(QPersistentModelIndex(mapToSource(playhead())));
    if (!switch_to(QPersistentModelIndex(mapFromSource(src_next)), true)) {
        m_d->playing->pause(); 
    }
}

void PlayQueueLI::handle_track_changed () {
    emit trackChanged();
    preload_next_track_whenever_possible();
}