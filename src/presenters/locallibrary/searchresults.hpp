#pragma once

#include "playlistsequence.hpp"

#include "searchresults-in.hpp"

// forward declaration
class SearchResultsLIPrivate;

class SearchResultsLI : public PlaylistSequence, public SearchResults
{
    Q_OBJECT
    Q_INTERFACES(SearchResults)

public:
    explicit SearchResultsLI(QObject *parent);

    Q_INVOKABLE void performSearch(const QString &query, QObject *sourceModel) override;
    void performSearch (const QString &query, QList<Types::Song> &song_list);

private:

    std::shared_ptr<SearchResultsLIPrivate> m_internal;
};