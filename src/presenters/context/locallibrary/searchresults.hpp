#pragma once

#include "playlistsequence.hpp"

#include <unicode/utypes.h>

class QQmlEngine;
class QJSEngine;

// forward declaration
U_NAMESPACE_BEGIN
class Transliterator;
U_NAMESPACE_END

class SearchResults : public PlaylistSequence {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // disable copy and assignment for single instance
    SearchResults(const SearchResults&) = delete;
    SearchResults &operator=(const SearchResults&) = delete;

    // singleton instantiation and global access
    static SearchResults &instance();
    static SearchResults *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    static std::string nfkd_and_translit (const std::string &string, icu::Transliterator* transliterator);

    Q_INVOKABLE void performSearch(const QString &query, QObject *sourceModel);
    void performSearch (const QString &query, QList<Types::Song> &song_list);

private:
    // private constructor to disallow external creations
    explicit SearchResults(QObject *parent = nullptr);

};