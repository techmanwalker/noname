#pragma once

#include "playlistsequence.hpp"

#include "searchresults-in.hpp"

#include <unicode/utypes.h>

// forward declaration
U_NAMESPACE_BEGIN
class Transliterator;
U_NAMESPACE_END

class SearchResultsLI : public PlaylistSequence, public SearchResults
{
    Q_OBJECT
    Q_INTERFACES(SearchResults)

public:
    explicit SearchResultsLI(QObject *parent);

    static std::string nfkd_and_translit (const std::string &string, icu::Transliterator* transliterator);

    Q_INVOKABLE void performSearch(const QString &query, QObject *sourceModel) override;
    void performSearch (const QString &query, QList<Types::Song> &song_list);

};