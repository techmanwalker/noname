#pragma once

#include "attributesstruct.hpp"
#include "songfactory.hpp"

namespace song_factory {

template <list_of_sources sl>
QList<QFuture<Types::Song>>
progressive_extract (const sl &sources, attributes a)
{
    using value_type = typename sl::value_type;

    QList<QUrl> sources_to_read;

    if constexpr (std::is_same_v<value_type, QString>) {
        for (const QString &source : sources) {
            sources_to_read.append(QUrl::fromLocalFile(source));
        }
    } else {
        sources_to_read = sources;
    }

    return progressive_extract(sources_to_read, a);
}

template <list_of_sources sl>
QFuture<QList<Types::Song>>
batch_extract (const sl &sources, attributes a)
{
    using value_type = typename sl::value_type;

    QList<QUrl> sources_to_read;

    if constexpr (std::is_same_v<value_type, QString>) {
        for (const QString &source : sources) {
            sources_to_read.append(QUrl::fromLocalFile(source));
        }
    } else {
        sources_to_read = sources;
    }

    return batch_extract(sources_to_read, a);
}

}