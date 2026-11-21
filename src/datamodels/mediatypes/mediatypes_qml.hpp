#pragma once

// QML type registration of the core media types.

#include <QtQmlIntegration>

#include "mediatypes.hpp"

namespace Types {

struct TrackForeign {
    Q_GADGET
    QML_FOREIGN(Types::Track)
    QML_VALUE_TYPE(track)
};

struct AlbumForeign {
    Q_GADGET
    QML_FOREIGN(Types::Album)
    QML_VALUE_TYPE(album)
};

struct PlaylistForeign {
    Q_GADGET
    QML_FOREIGN(Types::Playlist)
    QML_VALUE_TYPE(playlist)
};

struct DirectoryForeign {
    Q_GADGET
    QML_FOREIGN(Types::Directory)
    QML_VALUE_TYPE(directory)
};

}
