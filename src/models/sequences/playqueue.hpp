#pragma once

#include <QtQmlIntegration/qqmlintegration.h>
#include "playlistsequence.hpp"

// NOTE: use this as reference to implement singleton models inherited from non-singletons.

class QQmlEngine;
class QJSEngine;

// Media that will play up next.
class PlayQueue : public PlaylistSequence {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // disable copy and assignment for single instance
    PlayQueue(const PlayQueue&) = delete;
    PlayQueue &operator=(const PlayQueue&) = delete;

    // singleton instantiation and global access
    static PlayQueue &instance();
    static PlayQueue *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

private:
    // private constructor to disallow external creations
    explicit PlayQueue(QObject *parent = nullptr);
};