#pragma once

#include <QFuture>

class PlaylistsList
{

public:
    virtual ~PlaylistsList () = default;


};

Q_DECLARE_INTERFACE(PlaylistsList, "com.noname.PlaylistsList")