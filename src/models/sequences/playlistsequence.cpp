#include "playlistsequence.hpp"
#include "abstractmediasequence.hpp"
#include "defaultroles.hpp"
#include "src/playbackcontroller/songfactory.hpp"

#include <QFuture>
#include <QList>
#include <qfuture.h>
#include <qlist.h>

PlaylistSequence::PlaylistSequence(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}

PlaylistSequence::PlaylistSequence(
    QList<QUrl> sources_to_build_from,
    QFuture<void> *loading_finished_future,
    QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{
    // When loading is fully done, this QFuture will solve.
    QFuture<void> waiting_for_loaded = batch_append(sources_to_build_from);

    if (loading_finished_future != nullptr) *loading_finished_future = waiting_for_loaded;
}

void PlaylistSequence::append(const Types::Song &song) { AbstractMediaSequence::append(song); }
void PlaylistSequence::remove(int index)               { AbstractMediaSequence::remove(index); }
void PlaylistSequence::clear()                         { AbstractMediaSequence::clear(); }
void PlaylistSequence::items()                         { AbstractMediaSequence::items(); }

void
PlaylistSequence::batch_append(const QList<Types::Song> &songs) { AbstractMediaSequence::batch_append(songs);}

/// Append songs in batch with async loading
QFuture<void>
PlaylistSequence::batch_append(const QList<QUrl> &sources)
{
    // Build a list of the metadata extraction requests for each source
    QList<QFuture<Types::Song>> requests;
    requests.reserve(sources.size());

    // so we can know where it finishes
    auto promise = std::make_shared<QPromise<void>>();
    QFuture<void> future = promise->future();

    for (const QUrl &source : sources) {
        requests.append(
            song_factory::extract(source)
        );
    }
    // Monitor the entire batch without blocking the main thread
    // Pass the iterators and explicit type to whenAll.
    // Only pass the lambda expression as single argument to .then().
    // The parameter 'extracted_songs_ready_to_append' will contain the identical
    // list we built as input, ready to get the results from.
    QtFuture::whenAll<QList<QFuture<Types::Song>>>(requests.begin(), requests.end())
        .then(this, [this](const QList<QFuture<Types::Song>> &fetched_metadata_of_songs) {
            // Create final list for batch append
            QList<Types::Song> extracted_songs_ready_to_append;
            extracted_songs_ready_to_append.reserve(fetched_metadata_of_songs.size());

            for (const QFuture<Types::Song> &fetched_song_metadata : fetched_metadata_of_songs) {
                extracted_songs_ready_to_append.append(fetched_song_metadata.result());
            }

            // Massive and safe insertion on the sequence model (main thread)
            batch_append(extracted_songs_ready_to_append);
        })
        .then(this, [promise]() {
            promise->finish();
        });

    return future;
}