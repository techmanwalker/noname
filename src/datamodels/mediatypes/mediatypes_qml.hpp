#pragma once

// The one file in this module that's allowed to see QML's registration
// machinery. Kept out of mediatypes.hpp/.cpp on purpose: those headers are
// included by non-QML consumers (audioengine_li, songfactory, debugtools,
// playerpresenter_li) and shouldn't gain a QtQml include just because this
// module also happens to register its types for QML.
#include <QtQmlIntegration>

#include "mediatypes.hpp"

namespace Types {

struct SongForeign {
    Q_GADGET
    QML_FOREIGN(Types::Song)
    QML_VALUE_TYPE(song)
};

struct AlbumForeign {
    Q_GADGET
    QML_FOREIGN(Types::Album)
    QML_VALUE_TYPE(playlist)
};

struct DirectoryForeign {
    Q_GADGET
    QML_FOREIGN(Types::Directory)
    QML_VALUE_TYPE(directory)
};

}