#pragma once

#include "serialize.hpp"

namespace debug {

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

template <typename MediaType>
requires std::is_convertible_v<MediaType, Types::Any>
QJsonArray serialize(const QList<MediaType> &media)
{
    QList<Types::Any> any_list;

    for (const Types::Any &i : media) {
        any_list.append(i);
    }

    return serialize(any_list);
}

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