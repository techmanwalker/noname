#pragma once

#include <QFuture>

#include "interfaceid.hpp"
#include "mediatypes.hpp"

class PlaylistsList
{

public:
    virtual ~PlaylistsList () = default;

    virtual QList<Types::Playlist> items () const = 0;

    virtual QFuture<void> refresh () = 0;
    
    virtual QFuture<bool> save_playlist_to_disk (const QString &name, QList<QUrl> sources) = 0;
    virtual QFuture<bool> delete_playlists      (const QStringList &names)                 = 0;

    virtual bool pointed_to (const QPersistentModelIndex &idx, std::function<bool(const Types::Playlist &)> callback) = 0;
};

inline constexpr const char* PlaylistsList_iid = interface_iid<"PlaylistsList">.buf;
Q_DECLARE_INTERFACE(PlaylistsList, PlaylistsList_iid)
