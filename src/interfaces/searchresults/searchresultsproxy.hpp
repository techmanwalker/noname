#pragma once

#include "searchresults-in.hpp"

#include <QIdentityProxyModel>
#include <QtQmlIntegration>

class SearchResultsProxy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SearchResults)
    QML_SINGLETON

public:
    explicit SearchResultsProxy (QObject *parent = nullptr)
        : QObject (parent),
          m_search(s_injectedSearch) // Copies shared_ptr, incrementing ref count
    {}

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<SearchResults> &searchengine) {
        s_injectedSearch = searchengine; // Ref count incremented, caller's instance unaffected
    }

    Q_INVOKABLE QList<Types::Song> performSearch(const QString &query, QObject *containerModel) {
        if (!m_search) return {};
        
        return m_search->performSearch(query, containerModel);
    }

private:
    std::shared_ptr<SearchResults> m_search;
    inline static std::shared_ptr<SearchResults> s_injectedSearch = nullptr;
};