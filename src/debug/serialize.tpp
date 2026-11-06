#pragma once

#include "serialize.hpp"
#include <qjsonarray.h>

namespace debug {

template <media_type MediaType>
QJsonObject
serialize (const QFuture<MediaType> &future_unit)
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

template <media_sequence container>
QJsonArray
serialize (const container &media)
{
    QJsonArray out;

    for (const auto &unit : media) {
        out.append(serialize(unit));
    }

    return out;
}

template <media_type MediaType>
QJsonArray
serialize(const QList<QFuture<MediaType>> &futures)
{
    QJsonArray out;

    for (const auto &future : futures) {
        out.append(serialize(future));
    }

    return out;
}

template <media_type T>
QJsonObject
serialize (const T &unit)
{
    QJsonObject out;

    if (std::holds_alternative<Types::Song>(unit)) {
        out = serialize(std::get<Types::Song>(unit));
    }
    if (std::holds_alternative<Types::Album>(unit)) {
        out = serialize(std::get<Types::Album>(unit));
    }

    if (std::holds_alternative<Types::Directory>(unit)) {
        out = serialize(std::get<Types::Directory>(unit));
    }

    return out;
}

}