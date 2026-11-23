#pragma once

#include <QFuture>

#include "mediatypes.hpp"

namespace covers::live {
    class cover_provider;
}

namespace song_factory {

template <typename T>
concept list_of_sources =
    std::ranges::forward_range<T> && // Any type of list of sources

    // any uri or path
    (std::is_same_v<std::ranges::range_value_t<T>, QString> ||
     std::is_same_v<std::ranges::range_value_t<T>, QUrl>);

// A single file
QFuture<Types::Track> extract(const QUrl &source);


template <list_of_sources sl>
QList<QFuture<Types::Track>>
progressive_extract (const sl &sources);

// Any type of list of sources, wait for them all to finish extracting
template <list_of_sources sl>
QFuture<QList<Types::Track>>
batch_extract (const sl &sources);

// non template overloads to hide songfactoryimpl.hpp

QList<QFuture<Types::Track>>
progressive_extract (const QList<QUrl> &sources);

QFuture<QList<Types::Track>>
batch_extract (const QList<QUrl> &sources);

void teardown ();

}

#include "songfactory.tpp"
