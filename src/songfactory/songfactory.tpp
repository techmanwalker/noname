#pragma once

#include "songfactory.hpp"

template <typename Container>
requires
std::ranges::forward_range<Container> // Any type of list
&&  (
        std::is_same_v<typename Container::value_type, QString> // any uri or path
    ||  std::is_same_v<typename Container::value_type, QUrl>)
QList<QFuture<Types::Song>>
song_factory::progressive_extract (const Container &sources, std::shared_ptr<cover_provider> provider)
{
    // You can manually wait for each one to finish
    QList<QFuture<Types::Song>> requests;
    requests.reserve(sources.size());

    for (const QUrl &source : sources) {
        requests.append(song_factory::extract(source, provider));
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
song_factory::batch_extract (const Container &sources, std::shared_ptr<cover_provider> provider)
{
    auto requests = progressive_extract(sources, provider);

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
                qCWarning(l_songfactory) << "Skipping one song in batch: its extraction was canceled.";
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