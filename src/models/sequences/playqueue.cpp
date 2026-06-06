#include "playqueue.hpp"
#include <QQmlEngine>

// Meyers singleton implementation
PlayQueue &
PlayQueue::instance()
{
    static PlayQueue s_instance;
    return s_instance;
}

// factory for the qml engine
PlayQueue *
PlayQueue::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    
    PlayQueue *inst = &instance();

    // avoid QML GC to try to free object memory
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}

PlayQueue::PlayQueue(QObject *parent)
    : PlaylistSequence(parent)
{
}