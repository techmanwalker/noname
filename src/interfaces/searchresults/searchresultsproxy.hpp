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
          m_slist(s_injectedList) // Copies shared_ptr, incrementing ref count
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
        if (auto *list = qobject_cast<SearchResults*>(sourceModel())) {
            qDebug() << "SearchResultst was set.";
            return list->performSearch(query, containerModel);
        } else {
            qDebug() << "SearchResultst was NOT set.";
        }
    }

private:
    std::shared_ptr<QAbstractListModel> m_slist;
    inline static std::shared_ptr<QAbstractListModel> s_injectedList = nullptr;
};