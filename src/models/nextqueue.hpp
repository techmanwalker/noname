#pragma once

#include <QtQmlIntegration/qqmlintegration.h>
#include "playlistmodel.hpp"

// NOTE: use this as reference to implement singleton models inherited from non-singletons.

class QQmlEngine;
class QJSEngine;

class NextQueue : public PlaylistModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // disable copy and assignment for single instance
    NextQueue(const NextQueue&) = delete;
    NextQueue &operator=(const NextQueue&) = delete;

    // singleton instantiation and global access
    static NextQueue &instance();
    static NextQueue *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

private:
    // private constructor to disallow external creations
    explicit NextQueue(QObject *parent = nullptr);
};