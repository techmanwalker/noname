#pragma once

#include "virtual/mediafilter.hpp"

#include "objectproxybase.hpp"
#include <QtQmlIntegration>

// QObject MUST stay the first base: the QML prototype is resolved from it
class MediaFilterProxy : public QObject,
                         public ObjectProxyBase<MediaFilterProxy, MediaFilter>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MediaFilter)
    QML_SINGLETON

public:
    explicit MediaFilterProxy(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE search_response search(const QString &query, QObject *containerModel) {
        return call(&MediaFilter::search, query, containerModel);
    }
};
