#pragma once

#include "virtual/mediafilter.hpp"

#include "objectproxybase.hpp"

#include <QHash>
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

    // The result arrives through searchFinished; the id lets QML discard stale answers
    Q_INVOKABLE quint64 search(const QString &query, QObject *containerModel) {
        const quint64 id = ++m_lastRequestId;
        auto future = call(&MediaFilter::search, query, containerModel);
        m_inflight.insert(id, future);
        future.then(this, [this, id](const search_response &response) {
            m_inflight.remove(id);
            QMetaObject::invokeMethod(this, [this, id, response] {
                emit searchFinished(id, response);
            }, Qt::QueuedConnection);
        });
        return id;
    }

    // Drops the request's pending work; its answer is never emitted
    Q_INVOKABLE void cancelSearch(quint64 id) {
        const auto it = m_inflight.find(id);
        if (it == m_inflight.end()) return;
        it->cancelChain();
        m_inflight.erase(it);
    }

signals:
    void searchFinished(quint64 requestId, const search_response &response);

private:
    quint64 m_lastRequestId {};
    QHash<quint64, QFuture<search_response>> m_inflight;
};
