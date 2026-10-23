#pragma once

#include "mediatypes.hpp"
#include "playqueue-in.hpp"

#include <QIdentityProxyModel>
#include <QFuture>

#include <memory>
#include <qabstractitemmodel.h>

namespace covers::live {
    class cover_provider;
}

class PlayQueueLIPrivate;
class audio_engine;

// Media that will play up next.
class PlayQueueLI : public QIdentityProxyModel, public PlayQueue
{
    Q_OBJECT
    Q_INTERFACES(PlayQueue)

public:
    explicit PlayQueueLI(
        QObject *parent,
        std::shared_ptr<audio_engine> controller);


    // Required in the header for std::unique_ptr to destroy the incomplete type
    ~PlayQueueLI() override;

    // getters
    QPersistentModelIndex playhead() override;

    // all items
    QList<Types::Song> items() const;

    int item_count () const override;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false);
    bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false) override;
    void qml_switch_to (const QModelIndex &index);

    void switch_to (const QUrl &source) override;

    void next () override;
    void prev (bool play_afterwards = false) override;
    void prev_at_end (quint64 stepback_ms, bool play_afterwards = false) override;

    void clear () override;

    QPersistentModelIndex find_by_source (const QUrl &needle) override;
    std::optional<Types::Song> query_metadata_for_source (const QUrl &needle) const override;

    QFuture<void> batch_append (const QList<QUrl> &sources) override;

    bool shuffled () const override;
    void set_shuffle (bool enabled) override;

    void respawn_queue (const QList<Types::Song> &new_queue);
    QFuture<void> spawn_queue (const QStringList &sources);
    QFuture<void> respawn_queue (const QStringList &sources) override;

signals:
    void track_changed ();
    void count_changed ();
    void shuffle_changed ();

public slots:
    void preload_next_track_whenever_possible ();
    void handle_queued_tracks_finished ();
    void handle_track_changed ();

private:
    // PlayQueueLI space <-> PlaylistSequence space (two hops through ShuffleProxy)
    QModelIndex           to_sequence   (const QModelIndex &mine) const;
    QPersistentModelIndex from_sequence (const QModelIndex &seq)  const;

    // what comes after `mine` in the order the user *sees*
    QPersistentModelIndex successor_of (const QPersistentModelIndex &mine) const;

    std::unique_ptr<PlayQueueLIPrivate> m_d;
};