#pragma once

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

    Q_PROPERTY(QModelIndex playhead READ playhead WRITE qml_switch_to NOTIFY playheadChanged)

public:
    // disable copy and assignment for single instance
    PlayQueue(const PlayQueue&) = delete;
    PlayQueue &operator=(const PlayQueue&) = delete;

    // singleton instantiation and global access
    static PlayQueue &instance();
    static PlayQueue *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // getters
    QModelIndex playhead() const;
    int itemCount () const;

    // all items
    QList<Types::Song> items() const;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false); // no matter if it is on the queue or not
    void switch_to (const QPersistentModelIndex &song, bool play_afterwards = false); // for lvalues
    void switch_to (const QModelIndex &index, bool play_afterwards = false); // base for QML and temporary indices
    void qml_switch_to (const QModelIndex &index); // proxy for qml that auto plays the selection afterwards

    Q_INVOKABLE void next ();
    Q_INVOKABLE void prev ();

    // to drag and drop lists of songs on the qml gui
    Q_INVOKABLE QFuture<void> batch_append (const QList<QUrl> &sources);

    /// clear and repopulate the play queue in one step
    void respawn_queue (const QList<Types::Song> &new_queue);
    void respawn_queue (const PlaylistSequence &new_queue);

    // where are the covers for drag and drop appends saved?
    std::shared_ptr<cover_provider> chosen_cover_provider;

signals:
    void playheadChanged(bool play_afterwards = false);

private:
    // private constructor to disallow external creations
    explicit PlayQueue(QObject *parent = nullptr);

    QPersistentModelIndex m_playhead;

};
