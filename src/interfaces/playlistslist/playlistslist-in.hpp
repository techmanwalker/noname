#pragma once

#include <QFuture>

#include "interfaceid.hpp"

class PlaylistsList
{

public:
    virtual ~PlaylistsList () = default;


};

inline constexpr const char* PlaylistsList_iid = interface_iid<"PlaylistsList">.buf;
Q_DECLARE_INTERFACE(PlaylistsList, PlaylistsList_iid)