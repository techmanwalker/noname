#pragma once

#include "attributesstruct.hpp"
#include "songfactory.hpp"

namespace song_factory {

namespace {

template <list_of_sources sl>
QList<QUrl> string_file_paths_to_qurllist (const sl &sources)
{
    using value_type = typename sl::value_type;

    if constexpr (std::is_same_v<value_type, QString>) {
        QList<QUrl> local_file_paths;
        local_file_paths.reserve(sources.size());

        for (const QString &source : sources) {
            local_file_paths.append(QUrl::fromLocalFile(source));
        }

        return local_file_paths;
    } else {
        return sources;
    }
}

}

template <list_of_sources sl>
QList<QFuture<Types::Song>>
progressive_extract (const sl &sources, attributes a)
{

    return progressive_extract(
        string_file_paths_to_qurllist(sources), 
        a
    );
}

template <list_of_sources sl>
QFuture<QList<Types::Song>>
batch_extract (const sl &sources, attributes a)
{

    return batch_extract(
        string_file_paths_to_qurllist(sources),
        a
    );
}

}