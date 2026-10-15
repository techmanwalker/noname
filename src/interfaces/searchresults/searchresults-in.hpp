#pragma once

#include <QList>

namespace Types {
    struct Song;
}

class SearchResults
{

public: 
    virtual ~SearchResults () = default;

    virtual QList<Types::Song> performSearch(const QString &query, QObject *sourceModel) = 0;
};