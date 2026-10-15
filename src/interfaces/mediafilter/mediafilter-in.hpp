#pragma once

#include <QList>

namespace Types {
    struct Song;
}

class MediaFilter
{

public: 
    virtual ~MediaFilter () = default;

    virtual QList<Types::Song> performSearch(const QString &query, QObject *sourceModel) = 0;
};