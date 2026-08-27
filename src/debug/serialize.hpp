#pragma once

#include "mediatypes.hpp"

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <qjsonarray.h>
#include <type_traits>

template <typename T>
concept convertible_to_any_t = std::is_convertible_v<T, Types::Any>;

namespace debug {
    void print (const QLoggingCategory &cat, const QJsonValue &val);

    QJsonObject serialize(const Types::Song &song);
    QJsonObject serialize(const Types::Album &album);
    QJsonObject serialize(const Types::Directory &dir);
    QJsonObject serialize(const Types::Any &unit);

    QJsonArray serialize(const std::vector<std::string> &lines);

    template <convertible_to_any_t MediaType>
    QJsonObject serialize (const QFuture<MediaType> &future_unit);

    QJsonArray serialize(const QList<QUrl> &uris);
    QJsonArray serialize(const QList<Types::Any> &media);

    template <convertible_to_any_t MediaType>
    QJsonArray serialize(const QList<MediaType> &media);

    template <convertible_to_any_t MediaType>
    QJsonArray serialize(const QList<QFuture<MediaType>> &futures);
}

#include "serialize.tpp"