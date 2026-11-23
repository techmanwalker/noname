#include "basicdiskio.hpp"
#include "coveruris.hpp"
#include "virtual/manager.hpp"
#include "defaultroles.hpp"
#include "locallibrary.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"
#include <malloc.h>

QList<Types::Track>
LocalLibraryLI::flattened () const
{
    auto dirs= items<Types::Directory>();

    QList<Types::Track> flattened_directories_tracks;
    flattened_directories_tracks.reserve(dirs.size());

    for (const Types::Directory &dir : dirs) { // quick conversion to avoid more boilerplate
        flattened_directories_tracks.append(dir.tracks);
    }

    return flattened_directories_tracks;
}

QStringList
LocalLibraryLI::flattened_sources () const
{
    return AbstractMediaSequence::sources<QList<Types::Track>>(flattened());
}

QFuture<void>
LocalLibraryLI::take_snapshot (const QString &dir_path)
{

    // Try to search if the directory already has a snapshot
    QPersistentModelIndex dir_index_to_refresh = AbstractMediaSequence::find(&Types::Directory::path, dir_path);

    std::optional<size_t> dir_index_opt = dir_index_to_refresh.row();

    // this is how we create an empty directory reference
    if (!dir_index_to_refresh.isValid() || !dir_index_opt.has_value()) {
        qCDebug(l_mediasequences) << "No snapshot in memory for this directory. Loading...";

        dir_index_to_refresh =
            append(Types::Directory {
                { .title=QUrl(dir_path).fileName() },
                dir_path
            });

        dir_index_opt = dir_index_to_refresh.row();
    }

    if (!dir_index_to_refresh.isValid() || !dir_index_opt.has_value()) {

        qCFatal (l_mediasequences) << "Could not create a simple empty directory snapshot on LocalLibraryLI queue. "
            << "This is a fatal error. Aborting.";
    }

    QFuture<QList<Types::Track>> extract_future = QtFuture::makeReadyValueFuture<QList<Types::Track>>({});

    pointed_to<Types::Directory &>(
        dir_index_to_refresh,
        [&dir_path, &extract_future](Types::Directory &target_dir) -> void {
            // Recursively list files in directory
            QStringList files_in_path = diskio::list_dir(dir_path, true);

            // filter only tracks that don't exist already in found_dir.tracks
            QSet<QUrl> already_loaded;
            already_loaded.reserve(target_dir.tracks.size());
            for (const Types::Track &existing_song : std::as_const(target_dir.tracks)) {
                already_loaded.insert(existing_song.source);
            }

            QList<QUrl> not_yet_loaded;
            for (const QString &raw_source : files_in_path) {
                QUrl one_source = QUrl::fromLocalFile(raw_source);

                if (!already_loaded.contains(one_source)) {
                    not_yet_loaded.append(std::move(one_source));
                }
            }

            // avoid unnecessarily triggering a thread if everything is up to date
            if (not_yet_loaded.isEmpty()) {
                return;
            }

            qCDebug(l_mediasequences) << "Reading directory: " << target_dir.path;

            extract_future = song_factory::batch_extract (
                not_yet_loaded,
                {covers::default_cover_card_size.width(), true} /* enough for 1440p, can be tweaked any time later */
            );
        }
    );

    // Ask song_factory to extract the metadata of tracks, all at once
    return extract_future.then(this, [this, dir_index_to_refresh](QList<Types::Track> tracks) {

            //  re-resolve the target directory in case it has been modified in anyway while pointed_to remained unlocked
            auto did_execute = pointed_to<Types::Directory &>(
                dir_index_to_refresh,
                [&tracks](Types::Directory &target_dir) -> bool {
                    target_dir.tracks.reserve(target_dir.tracks.size() + tracks.size());

                    // source -> position in target_dir.tracks, kept in sync as we append
                    QHash<QUrl, qsizetype> index_of;
                    index_of.reserve(target_dir.tracks.size() + tracks.size());
                    for (qsizetype i = 0; i < target_dir.tracks.size(); ++i) {
                        index_of.insert(target_dir.tracks.at(i).source, i);
                    }

                    // recheck if each track was truly not added yet; if so, overwrite it
                    for (Types::Track &song : tracks) {
                        // A song with empty source is considered invalid
                        if (!song.is_valid()) {
                            continue;
                        }

                        auto it = index_of.constFind(song.source);
                        if (it != index_of.constEnd()) {
                            target_dir.tracks[it.value()] = std::move(song);
                        } else {
                            index_of.insert(song.source, target_dir.tracks.size());
                            target_dir.tracks.append(std::move(song));
                        }
                    }

                    return true;
                }
            );

            if (did_execute) {
                emit dataChanged(dir_index_to_refresh, dir_index_to_refresh);
            } else {
                qCDebug(l_mediasequences) << "Directory was unmapped while metadata extraction was in progress.";
            }
        })
        .onFailed(this, [this] {
            qCWarning(l_mediasequences) << "Batch song load failed for a directory; its tracks list was left unchanged.";
        });
}

QFuture<void>
LocalLibraryLI::take_snapshots (const QStringList &paths)
{
    QList<QFuture<void>> snapshot_futures;
    snapshot_futures.reserve(paths.size());

    for (const QString &dir_path : paths) {
        snapshot_futures.append(
            take_snapshot(dir_path)
        );
    }

    return QtFuture::whenAll(snapshot_futures.begin(), snapshot_futures.end());
}

QFuture<void>
LocalLibraryLI::retake_all_snapshots ()
{
    return take_snapshots(paths())
    .then(this, [this]() {
        emit refreshFinished();
    });
}

QFuture<void>
LocalLibraryLI::snapshot_known_directories ()
{
    // This is a full refresh.

    using st = configuration::setting;

    return take_snapshots(
        cm->fetch_setting(
            st::known_music_directories
        ).value_or({})
    ).then(this, [this]() {
        // trim up the massive thread memory burden created by the extraction
        malloc_trim(0);
        emit refreshFinished();
    });
}

QStringList
LocalLibraryLI::paths ()
{
    // .path of all items that are Types::Directory
    QList<Types::Directory> dir_items = items<Types::Directory>();

    QStringList dir_paths;
    dir_paths.reserve(dir_items.size());

    for (Types::Directory &dir : dir_items) {
        dir_paths.push_back(std::move(dir.path));
    }

    return dir_paths;
}

LocalLibraryLI::LocalLibraryLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager)
    : AbstractMediaSequence (parent, container_roles),
      cm(std::move(confmanager))
{
}
