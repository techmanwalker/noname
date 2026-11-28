#pragma once

#include "mediatypes.hpp"
#include "virtual/playlistslist.hpp"
#include "virtual/playqueue.hpp"

#include <QIdentityProxyModel>
#include <QFuture>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(l_playqueue)

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
        std::shared_ptr<audio_engine> audioengine,
        std::shared_ptr<PlaylistsList> playlistslist
    );


    // Required in the header for std::unique_ptr to destroy the incomplete type
    ~PlayQueueLI() override;

    // getters
    QPersistentModelIndex playhead() const override;

    // all items
    QList<Types::Track> items() const;

    // or their sources
    QList<QUrl> sources() const;

    int item_count () const override;

    // controls
    [[nodiscard]] QFuture<void> switch_to (const Types::Track &song, bool play_afterwards = false);
    [[nodiscard]] QFuture<void> switch_to (const QPersistentModelIndex &song, bool play_afterwards = false) override;
    [[nodiscard]] QFuture<void> switch_to (size_t idx, bool play_afterwards = false) override;
    [[nodiscard]] QFuture<void> switch_to (const QUrl &source) override;
    QFuture<void> qml_switch_to (const QModelIndex &index);

    QFuture<void> next () override;
    QFuture<void> prev (bool play_afterwards = false) override;
    QFuture<void> prev_at_end (quint64 stepback_ms, bool play_afterwards = false) override;

    void clear () override;

    bool pointed_to (const QPersistentModelIndex &idx, std::function<bool(const Types::Track &)> callback) const override;

    QPersistentModelIndex find_by_source (const QUrl &needle) override;
    std::optional<Types::Track> query_metadata_for_source (const QUrl &needle) const override;

    QFuture<void> batch_append (const QList<QUrl> &sources) override;

    bool shuffled () const override;
    void set_shuffle (bool enabled) override;

    QFuture<void> spawn_queue   (const QStringList &sources);
    QFuture<void> respawn_queue (const QStringList &sources) override;
    void respawn_queue (QList<Types::Track> tracks) override;

    QFuture<bool> save_as_playlist_to_disk (const QString &name) override;

    bool move_item (const QPersistentModelIndex &src, const QPersistentModelIndex &dest) override;

    QFuture<void> batch_remove (const QList<QPersistentModelIndex> &indices) override;

signals:
    // to qml
    void track_changed ();
    void count_changed ();
    void shuffle_changed ();

public slots:
    QFuture<void> preload_next_track_whenever_possible ();
    QFuture<void> handle_tape_ran_out ();
    void handle_ab_boundary_crossed ();
    void handle_engine_track_changed ();

private slots:
    void handle_zero_item_count ();

private:
    // PlayQueueLI space -> source space (the identity hop, then the sequence namespace's)
    QModelIndex to_sequence (const QModelIndex &mine) const;

    // what comes after or before `mine` in the order the user *sees*
    QPersistentModelIndex predecessor_of (const QPersistentModelIndex &mine) const;
    QPersistentModelIndex successor_of   (const QPersistentModelIndex &mine) const;

    QPersistentModelIndex m_playhead;

    void set_playhead_  (const QPersistentModelIndex &to); // leaves the engine intact
    [[nodiscard]] QFuture<void> move_playhead_ (const QPersistentModelIndex &to, bool play_afterwards); // invokes ae->load
    void invalidate_playhead_ ();
    void assert_playhead_in_sync_ () const; // debug sentinel

    std::unique_ptr<PlayQueueLIPrivate> m_d;
};
