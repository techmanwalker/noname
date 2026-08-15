#pragma once

#include <QFuture>

#include "mediatypes.hpp"

struct attributes;

namespace covers::live {
    class cover_provider;
}

namespace song_factory {

// A single file
QFuture<Types::Song> extract(const QUrl &source, attributes a);

// Any type of list of sources
template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QList<QFuture<Types::Song>>
progressive_extract (const Container &sources, attributes a);

// Any type of list of sources, wait for them all to finish extracting
template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QFuture<QList<Types::Song>>
batch_extract (const Container &sources, attributes a);

// non template overloads to hide songfactoryimpl.hpp

QList<QFuture<Types::Song>>
progressive_extract (const QList<QUrl> &sources, attributes a);

QFuture<QList<Types::Song>>
batch_extract (const QList<QUrl> &sources, attributes a);

void shutdown ();

}

#include "songfactory.tpp"