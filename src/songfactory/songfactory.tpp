#pragma once

#include "attributesstruct.hpp"
#include "songfactory.hpp"

#include "songfactoryimpl.hpp"

namespace song_factory {

template <typename Container>
requires
std::ranges::forward_range<Container>
&&  (
        std::is_same_v<typename Container::value_type, QString>
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QList<QFuture<Types::Song>>
progressive_extract (const Container &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    using value_type = typename Container::value_type;

    QList<QUrl> sources_to_read;

    if constexpr (std::is_same_v<value_type, QString>) {
        for (const QString &source : sources) {
            sources_to_read.append(QUrl::fromLocalFile(source));
        }
    } else {
        sources_to_read = sources;
    }

    return progressive_extract(sources_to_read, provider, a);
}

template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QFuture<QList<Types::Song>>
batch_extract (const Container &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    using value_type = typename Container::value_type;

    QList<QUrl> sources_to_read;

    if constexpr (std::is_same_v<value_type, QString>) {
        for (const QString &source : sources) {
            sources_to_read.append(QUrl::fromLocalFile(source));
        }
    } else {
        sources_to_read = sources;
    }

    return batch_extract(sources_to_read, provider, a);
}

}