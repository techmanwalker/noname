#pragma once

#include <QtQmlIntegration/qqmlintegration.h>
#include <QAbstractItemModel>
#include <qabstractitemmodel.h>
#include <qtmetamacros.h>
#include "playlistsequence.hpp"

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

    // drag and drop files to the play queue
    Q_INVOKABLE QFuture<void> batch_append(const QList<QUrl> &sources);

    // getters
    QModelIndex playhead() const { return m_playhead; }
    int itemCount () const;

    // controls
    void switch_to (const Types::Song &song, bool play_afterwards = false); // no matter if it is on the queue or not
    void switch_to (const QPersistentModelIndex &song, bool play_afterwards = false); // for lvalues
    void switch_to (const QModelIndex &index, bool play_afterwards = false); // base for QML and temporary indices
    void qml_switch_to (const QModelIndex &index); // proxy for qml that auto plays the selection afterwards

    Q_INVOKABLE void next ();
    Q_INVOKABLE void prev ();

    // to find out if a song is in queue
    QPersistentModelIndex find (const Types::Song &needle) const;

    /// clear and repopulate the play queue in one step
    void respawn_queue (QList<QUrl> new_queue);
    void respawn_queue (PlaylistSequence &new_queue);

signals:
    void playheadChanged(bool play_afterwards = false);

private:
    // private constructor to disallow external creations
    explicit PlayQueue(QObject *parent = nullptr);

    QPersistentModelIndex m_playhead;
};