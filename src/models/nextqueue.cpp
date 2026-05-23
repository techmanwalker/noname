#include "nextqueue.hpp"
#include <QQmlEngine>

// Meyers singleton implementation
NextQueue &
NextQueue::instance()
{
    static NextQueue s_instance;
    return s_instance;
}

// factory for the qml engine
NextQueue *
NextQueue::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    
    NextQueue *inst = &instance();

    // avoid QML GC to try to free object memory
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}

NextQueue::NextQueue(QObject *parent)
    : PlaylistModel(parent)
{
}