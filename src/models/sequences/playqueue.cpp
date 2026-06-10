#include "playqueue.hpp"
#include "playlistsequence.hpp"
#include <QQmlEngine>
#include <qlist.h>

PlayQueue::PlayQueue(QObject *parent)
    : PlaylistSequence(parent)
{
}

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

void
PlayQueue::switch_queue(QList<QUrl> new_queue)
{
    clear();

    batch_append(new_queue);
}

void
PlayQueue::switch_queue(PlaylistSequence &new_queue)
{
    clear();

    batch_append(new_queue.items<Types::Song>());
}