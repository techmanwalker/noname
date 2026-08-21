#pragma once

#include <QFuture>

#include "mediatypes.hpp"

struct attributes;

namespace covers::live {
    class cover_provider;
}

namespace song_factory {

template <typename T>
concept Container_T = 
    std::ranges::forward_range<T> && // Any type of list of sources

    // any uri or path
    (std::is_same_v<std::ranges::range_value_t<T>, QString> ||  
     std::is_same_v<std::ranges::range_value_t<T>, QUrl>);

// A single file
QFuture<Types::Song> extract(const QUrl &source, attributes a);


template <Container_T Container>
QList<QFuture<Types::Song>>
progressive_extract (const Container &sources, attributes a);

// Any type of list of sources, wait for them all to finish extracting
template <Container_T Container>
QFuture<QList<Types::Song>>
batch_extract (const Container &sources, attributes a);

// non template overloads to hide songfactoryimpl.hpp

QList<QFuture<Types::Song>>
progressive_extract (const QList<QUrl> &sources, attributes a);

QFuture<QList<Types::Song>>
batch_extract (const QList<QUrl> &sources, attributes a);

void teardown ();

}

#include "songfactory.tpp"