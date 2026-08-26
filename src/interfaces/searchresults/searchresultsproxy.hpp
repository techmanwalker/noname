#pragma once

#include "searchresults-in.hpp"

#include <QIdentityProxyModel>

#include <QtQmlIntegration/qqmlintegration.h>

class SearchResultsProxy : public QIdentityProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(SearchResults)
    QML_SINGLETON

public:
    explicit SearchResultsProxy (QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_slist(s_injectedList), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<SearchResults*>(m_slist.get()))
    {
        if (m_slist) {
            setSourceModel(m_slist.get());
        }
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &list) {
        s_injectedList = list; // Ref count incremented, caller's instance unaffected
    }

    Q_INVOKABLE void performSearch(const QString &query, QObject *containerModel) {
        if (!m_iface) return;
        
        return m_iface->performSearch(query, containerModel);
    }

private:
    std::shared_ptr<QAbstractListModel> m_slist;
    inline static std::shared_ptr<QAbstractListModel> s_injectedList = nullptr;

    SearchResults *m_iface = nullptr;
};