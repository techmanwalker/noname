#pragma once

#include "mediatypes.hpp"

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <type_traits>

namespace debug {
    void print (const QLoggingCategory &cat, const QJsonValue &val);

    QJsonObject serialize(const Types::Song &song);
    QJsonObject serialize(const Types::Album &album);
    QJsonObject serialize(const Types::Directory &dir);
    QJsonObject serialize(const Types::Any &unit);

    template <typename MediaType>
    requires std::is_convertible_v<MediaType, Types::Any>
    QJsonObject serialize (const QFuture<MediaType> &future_unit)
    {
        QJsonObject out;

        bool finished = future_unit.isFinished();

        out["finished"] = finished;
        out["value"] = 
            finished ?
                serialize(future_unit.result()) :
                QJsonObject {};

        return out;
    }

    QJsonArray serialize(const QList<QUrl> &uris);
    QJsonArray serialize(const QList<Types::Any> &media);

    template <typename MediaType>
    requires std::is_convertible_v<MediaType, Types::Any>
    QJsonArray serialize(const QList<QFuture<MediaType>> &futures)
    {
        QJsonArray out;

        for (const auto &future : futures) {
            out.append(serialize(future));
        }

        return out;
    }
}