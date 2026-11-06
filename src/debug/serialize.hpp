#pragma once

#include "mediaconcepts.hpp"
#include "mediatypes.hpp"

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace debug {
    void print (const QLoggingCategory &cat, const QJsonValue &val);

    QJsonObject serialize(const Types::Song &song);
    QJsonObject serialize(const Types::Album &album);
    QJsonObject serialize(const Types::Directory &dir);
    template <media_type T>
    QJsonObject serialize(const T &unit);

    QJsonArray serialize(const std::vector<std::string> &lines);

    template <media_type MediaType>
    QJsonObject serialize (const QFuture<MediaType> &future_unit);

    QJsonArray serialize(const QList<QUrl> &uris);

    template <media_sequence container>
    QJsonArray serialize(const container &media);

    template <media_type MediaType>
    QJsonArray serialize(const QList<QFuture<MediaType>> &futures);
}

#include "serialize.tpp"