#pragma once

#include <QFuture>
#include <QPersistentModelIndex>

class PlayQueue
{

public: 
    virtual ~PlayQueue () = default;

    virtual int itemCount () const = 0;

    virtual QPersistentModelIndex playhead () = 0;

    virtual void switch_to (const QUrl &source) = 0;
    virtual bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false) = 0;

    virtual void next () = 0;
    virtual void prev () = 0;

    virtual void clear () = 0;

    virtual QPersistentModelIndex find_by_source (const QString &needle) = 0;

    virtual QFuture<void> batch_append (const QList<QUrl> &sources) = 0;
    
    virtual void respawn_queue (const QStringList &sources) = 0;
};

Q_DECLARE_INTERFACE(PlayQueue, "com.noname.PlayQueue")