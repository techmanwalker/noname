#pragma once

#include <QFuture>

#include "interfaceid.hpp"

class PlaylistsList
{

public:
    virtual ~PlaylistsList () = default;

    virtual QFuture<void> refresh () = 0;
    virtual QFuture<bool> save_playlist_to_disk (const QString &name, QList<QUrl> sources) = 0;
};

inline constexpr const char* PlaylistsList_iid = interface_iid<"PlaylistsList">.buf;
Q_DECLARE_INTERFACE(PlaylistsList, PlaylistsList_iid)
