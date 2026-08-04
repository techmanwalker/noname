#pragma once

#include "songfactory.hpp"

template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QList<QFuture<Types::Song>>
song_factory::progressive_extract (const Container &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    using value_type = typename Container::value_type;

    // Read the metadata of the sources one by one in parallel, you can do something as soon as one extraction finishes
    QList<QFuture<Types::Song>> requests;
    requests.reserve(sources.size());

    for (const value_type &raw_source : sources) {
        QUrl source;

        if constexpr (std::is_same_v<value_type, QString>) {
            
            // QStringList are filesystem paths in this player
            source = QUrl::fromLocalFile(raw_source);
        } else {
            source = raw_source;
        }

        requests.append(song_factory::extract(source, provider, a));
    }

    return requests;
}

template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QFuture<QList<Types::Song>>
song_factory::batch_extract (const Container &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    auto requests = progressive_extract(sources, provider, a);

    // Wait for all songs in the list to finish metadata extraction

    return QtFuture::whenAll(
        requests.begin(),
        requests.end()
    )
    .then([](const QList<QFuture<Types::Song>> &finished) {
        QList<Types::Song> extracted_songs_ready_to_append;

        extracted_songs_ready_to_append.reserve(finished.size());

        for (const auto &f : finished) {
            /*  Isolate one bad future's exception to just this iteration — otherwise a single
                failed extraction throws out of this loop and takes the whole batch's already-
                successful results down with it, since QFuture::result() rethrows whatever
                exception QtConcurrent::run captured on that future's worker thread. */
            if (f.isCanceled()) {
                continue;
            }

            // truly exceptional metadata extraction failures
            try {
                extracted_songs_ready_to_append.append(f.result());
            } catch (const std::exception &e) {
                qCWarning(l_songfactory) << "Skipping one song in batch: extraction failed with" << e.what();
            } catch (...) {
                qCWarning(l_songfactory) << "Skipping one song in batch: extraction failed with an unknown exception.";
            }
        }

        return extracted_songs_ready_to_append;
    });
}