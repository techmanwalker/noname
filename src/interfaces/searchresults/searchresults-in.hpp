#pragma once

#include <QObject>

namespace Types {
    struct Song;
}

class SearchResults
{

public: 
    virtual ~SearchResults () = default;

    virtual void performSearch(const QString &query, QObject *sourceModel) = 0;
};

Q_DECLARE_INTERFACE(SearchResults, "com.noname.SearchResults")