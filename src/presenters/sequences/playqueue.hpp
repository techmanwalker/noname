#pragma once

#include "audioengine.hpp"
#include "playlistsequence.hpp"

#include <QtQmlIntegration/qqmlintegration.h>

// NOTE: use this as reference to implement singleton models inherited from non-singletons.

class QQmlEngine;
class QJSEngine;

// Media that will play up next.
class PlayQueue : public PlaylistSequence {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QModelIndex playhead READ playhead WRITE qml_switch_to NOTIFY track_changed)

public:
    // disable copy and assignment for single instance
    PlayQueue(const PlayQueue&) = delete;
    PlayQueue &operator=(const PlayQueue&) = delete;

    // singleton instantiation and global access
    static PlayQueue &instance();
    static PlayQueue *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // getters
    QPersistentModelIndex playhead();

    // all items
    QList<Types::Song> items() const;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false); // clean the queue completely and repopulate with this song
    bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false); // for lvalues
    void qml_switch_to (const QModelIndex &index); // proxy for qml that auto plays the selection afterwards

    Q_INVOKABLE void switch_to (const QUrl &source);

    Q_INVOKABLE void next ();
    Q_INVOKABLE void prev ();

    Q_INVOKABLE void clear ();

    // to drag and drop lists of songs on the qml gui
    Q_INVOKABLE QFuture<void> batch_append (const QList<QUrl> &sources);

    /// clear and repopulate the play queue in one step
    void respawn_queue (const QList<Types::Song> &new_queue);
    void respawn_queue (const PlaylistSequence &new_queue);
    Q_INVOKABLE void respawn_queue (const QStringList &sources);

    // where are the covers for drag and drop appends saved?
    std::shared_ptr<covers::live::cover_provider> chosen_cover_provider;

signals:
    void track_changed ();

private slots:
    void preload_next_track_whenever_possible ();
    void handle_queued_tracks_finished ();
    void handle_track_changed ();

private:
    // private constructor to disallow external creations
    explicit PlayQueue(QObject *parent = nullptr);

    // the playhead is passively calculated
    audio_engine &playing = audio_engine::instance();

};
