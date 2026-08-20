#pragma once

#include "audiocontroller.hpp"
#include "mediatypes.hpp"

#include <QIdentityProxyModel>
#include <QFuture>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

class QQmlEngine;
class QJSEngine;

namespace covers::live {
    class cover_provider;
}

struct PlayQueuePrivate;

// Media that will play up next.
class PlayQueue : public QIdentityProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QModelIndex playhead READ playhead  WRITE qml_switch_to NOTIFY trackChanged)
    Q_PROPERTY(qsizetype   count    READ itemCount NOTIFY countChanged)

public:
    // disable copy and assignment for single instance
    PlayQueue(const PlayQueue&) = delete;
    PlayQueue &operator=(const PlayQueue&) = delete;

    // singleton instantiation and global access
    static PlayQueue &instance();
    static PlayQueue *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Required in the header for std::unique_ptr to destroy the incomplete type
    ~PlayQueue() override;

    // getters
    QPersistentModelIndex playhead();

    // all items
    QList<Types::Song> items() const;

    int itemCount () const;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false);
    bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false);
    void qml_switch_to (const QModelIndex &index);

    Q_INVOKABLE void switch_to (const QUrl &source);

    Q_INVOKABLE void next ();
    Q_INVOKABLE void prev ();

    Q_INVOKABLE void clear ();

    Q_INVOKABLE QFuture<void> batch_append (const QList<QUrl> &sources);

    void respawn_queue (const QList<Types::Song> &new_queue);
    Q_INVOKABLE void respawn_queue (const QStringList &sources);

    // Replaces the public property to keep the provider opaque
    void set_cover_provider(std::shared_ptr<covers::live::cover_provider> provider);
    void set_audio_controller(std::shared_ptr<audio_controller> controller);

signals:
    void trackChanged ();
    void countChanged ();

public slots:
    void preload_next_track_whenever_possible ();
    void handle_queued_tracks_finished ();
    void handle_track_changed ();

private:
    explicit PlayQueue(QObject *parent = nullptr);

    std::unique_ptr<PlayQueuePrivate> m_d;
};