#pragma once

#include <QList>

namespace Types {
    struct Song;
}

class MediaFilter
{

public: 
    virtual ~MediaFilter () = default;

    virtual QList<Types::Song> search (const QString &query, QObject *sourceModel) = 0;
};