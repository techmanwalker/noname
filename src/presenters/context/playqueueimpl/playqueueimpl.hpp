#pragma once

#include "mediatypes.hpp"
#include "playqueue.hpp"

#include <QIdentityProxyModel>
#include <QFuture>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

namespace covers::live {
    class cover_provider;
}

struct LI_PlayQueuePrivate;
class audio_engine;

// Media that will play up next.
class LI_PlayQueue : public QIdentityProxyModel, public PlayQueue
{
    Q_OBJECT
    Q_INTERFACES(PlayQueue)

public:
    explicit LI_PlayQueue(
        QObject *parent,
        std::shared_ptr<covers::live::cover_provider> provider,
        std::shared_ptr<audio_engine> controller);


    // Required in the header for std::unique_ptr to destroy the incomplete type
    ~LI_PlayQueue() override;

    // getters
    QPersistentModelIndex playhead() override;

    // all items
    QList<Types::Song> items() const;

    int itemCount () const override;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false);
    bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false) override;
    void qml_switch_to (const QModelIndex &index);

    Q_INVOKABLE void switch_to (const QUrl &source) override;

    Q_INVOKABLE void next () override;
    Q_INVOKABLE void prev () override;

    Q_INVOKABLE void clear () override;

    Q_INVOKABLE QFuture<void> batch_append (const QList<QUrl> &sources) override;

    void respawn_queue (const QList<Types::Song> &new_queue);
    Q_INVOKABLE void respawn_queue (const QStringList &sources) override;

signals:
    void trackChanged ();
    void countChanged ();

public slots:
    void preload_next_track_whenever_possible ();
    void handle_queued_tracks_finished ();
    void handle_track_changed ();

private:

    std::unique_ptr<LI_PlayQueuePrivate> m_d;
};