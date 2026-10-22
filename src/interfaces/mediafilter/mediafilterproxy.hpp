#pragma once

#include "mediafilter-in.hpp"

#include <QIdentityProxyModel>
#include <QtQmlIntegration>

class MediaFilterProxy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MediaFilter)
    QML_SINGLETON

public:
    explicit MediaFilterProxy (QObject *parent = nullptr)
        : QObject (parent),
          m_filter(s_injectedFilter) // Copies shared_ptr, incrementing ref count
    {}

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<MediaFilter> &searchengine) {
        s_injectedFilter = searchengine; // Ref count incremented, caller's instance unaffected
    }

    Q_INVOKABLE search_response search(const QString &query, QObject *containerModel) {
        if (!m_filter) return {};
        
        return m_filter->search(query, containerModel);
    }

private:
    std::shared_ptr<MediaFilter> m_filter;
    inline static std::shared_ptr<MediaFilter> s_injectedFilter = nullptr;
};