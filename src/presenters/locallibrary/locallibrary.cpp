#include "basicdiskio.hpp"
#include "coveruris.hpp"
#include "manager-in.hpp"
#include "defaultroles.hpp"
#include "locallibrary.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"
#include <qfuture.h>

QList<Types::Song>
LocalLibraryLI::flattened () const
{
    auto dirs= items<Types::Directory>();

    QList<Types::Song> flattened_directories_songs;
    flattened_directories_songs.reserve(dirs.size());

    for (const Types::Directory &dir : dirs) { // quick conversion to avoid more boilerplate
        flattened_directories_songs.append(dir.songs);
    }

    return flattened_directories_songs;
}

QStringList
LocalLibraryLI::flattened_sources () const
{
    return AbstractMediaSequence::sources<QList<Types::Song>>(flattened());
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
            append(Types::Directory (
                dir_path
            ));

        dir_index_opt = dir_index_to_refresh.row();
    }

    if (!dir_index_to_refresh.isValid() || !dir_index_opt.has_value()) {

        qCFatal (l_mediasequences) << "Could not create a simple empty directory snapshot on LocalLibraryLI queue. "
            << "This is a fatal error. Aborting.";
    }

    QFuture<QList<Types::Song>> extract_future = QtFuture::makeReadyValueFuture<QList<Types::Song>>({});
    
    pointed_to<Types::Directory &>(
        dir_index_to_refresh, 
        [&dir_path, &extract_future](Types::Directory &target_dir) -> void {
            // Recursively list files in directory
            QStringList files_in_path = diskio::list_dir(dir_path, true);

            // filter only songs that don't exist already in found_dir.songs
            QList<QUrl> not_yet_loaded;
            for (const QString &raw_source : files_in_path) {
                QUrl one_source = QUrl::fromLocalFile(raw_source);

                auto it = std::ranges::find_if(target_dir.songs, [&one_source](const Types::Song &existing_song) {
                    return existing_song.source == one_source;
                });

                if (it == target_dir.songs.end()) {
                    not_yet_loaded.append(one_source);
                }
            }

            // avoid unnecessarily triggering a thread if everything is up to date
            if (not_yet_loaded.isEmpty()) {
                return;
            }

            qCDebug(l_mediasequences) << "Reading directory: " << target_dir.path;

            extract_future = song_factory::batch_extract (
                not_yet_loaded, 
                {covers::default_cover_thumbnail_size, true} /* enough for 1440p, can be tweaked any time later */
            );
        }
    );

    // Ask song_factory to extract the metadata of songs, all at once
    return extract_future.then(this, [this, dir_index_to_refresh](QList<Types::Song> songs) {
            /*  Re-resolve through the persistent index rather than trusting target_dir from
                above: m_items may have reallocated (another take_snapshot() appending a new
                directory, for instance) during the time batch_extract spent running. */
            auto did_execute = pointed_to<Types::Directory &>(
                dir_index_to_refresh, [&songs](Types::Directory &target_dir) -> bool {
                    target_dir.songs.reserve(songs.size());

                    // Defensive merge-or-append: not_yet_loaded was deduped by source up front, but
                    // another in-flight take_snapshot() on this same directory could have loaded some
                    // of these same sources in the meantime.
                    for (Types::Song &song : songs) {
                        // A song with empty source is considered invalid
                        if (!song.is_valid()) {
                            continue;
                        }

                        auto it = std::ranges::find_if(target_dir.songs, [&song](const Types::Song &existing_song) {
                            return existing_song.source == song.source;
                        });

                        if (it != target_dir.songs.end()) {
                            *it = std::move(song);
                        } else {
                            target_dir.songs.append(std::move(song));
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
            qCWarning(l_mediasequences) << "Batch song load failed for a directory; its songs list was left unchanged.";
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

    using ft = configuration::conf_file_type;

    return take_snapshots(
        cm->read_lines(
            ft::known_music_directories
        )
    ).then(this, [this]() {
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
      cm(confmanager)
{
}