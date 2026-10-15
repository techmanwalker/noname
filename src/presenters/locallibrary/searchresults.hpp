#pragma once

#include "searchresults-in.hpp"

// forward declaration
class SearchResultsLIPrivate;

class SearchResultsLI : public SearchResults
{

public:
    explicit SearchResultsLI();

    QList<Types::Song> performSearch (const QString &query, QObject *sourceModel) override;
    QList<Types::Song> performSearch (const QString &query, QList<Types::Song> &song_list);

private:

    std::shared_ptr<SearchResultsLIPrivate> m_internal;
};