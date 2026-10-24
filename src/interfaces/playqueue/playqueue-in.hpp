#pragma once

#include <QFuture>
#include <QPersistentModelIndex>
#include <optional>
#include <qfuture.h>

#include "mediatypes.hpp"

class PlayQueue
{

public: 
    virtual ~PlayQueue () = default;

    virtual int item_count () const = 0;

    virtual QPersistentModelIndex playhead () = 0;

    virtual void switch_to (const QUrl &source) = 0;
    virtual bool switch_to (const QPersistentModelIndex &song, bool play_afterwards = false) = 0;
    virtual bool switch_to (size_t index, bool play_afterwards = false) = 0;

    virtual bool next () = 0;
    virtual bool prev (bool play_afterwards = false) = 0;

    // play the previous track, but seek backward from the end
    virtual bool prev_at_end (quint64 stepback_ms, bool play_afterwards = false) = 0;

    virtual void clear () = 0;

    virtual QPersistentModelIndex find_by_source (const QUrl &needle) = 0;
    virtual std::optional<Types::Song> query_metadata_for_source (const QUrl &needle) const = 0;

    virtual QFuture<void> batch_append (const QList<QUrl> &sources) = 0;
    
    virtual QFuture<void> respawn_queue (const QStringList &sources) = 0;
    virtual void respawn_queue (const QList<Types::Song> &songs) = 0;

    virtual bool shuffled () const = 0;
    virtual void set_shuffle (bool enabled) = 0;
};

Q_DECLARE_INTERFACE(PlayQueue, "com.noname.PlayQueue")