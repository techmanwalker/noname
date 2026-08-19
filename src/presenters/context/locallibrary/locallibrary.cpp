#include "basicdiskio.hpp"
#include "configuration.hpp"
#include "defaultroles.hpp"
#include "locallibrary.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"

LocalLibrary &
LocalLibrary::instance ()
{
    static LocalLibrary s_instance;
    return s_instance;
}

LocalLibrary *
LocalLibrary::create (QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    LocalLibrary *inst = &instance();

    // avoid QML GC to try to free object memory
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}

QList<Types::Song>
LocalLibrary::flattened () const
{
    QList<Types::Song> flattened_directories_songs;

    for (const Types::Directory &dir : items()) { // quick conversion to avoid more boilerplate
        flattened_directories_songs.append(dir.songs);
    }

    return flattened_directories_songs;
}

QStringList
LocalLibrary::flattened_sources () const
{
    return AbstractMediaSequence::sources<QList<Types::Song>>(flattened());
}

QFuture<void>
LocalLibrary::take_snapshot (const QString &dir_path)
{

    // Try to search if the directory already has a snapshot
    QPersistentModelIndex dir_index_to_refresh = AbstractMediaSequence::find(&Types::Directory::path, dir_path);

    std::optional<size_t> dir_index_opt = row_pointed_to(dir_index_to_refresh);

    // this is how we create an empty directory reference
    // row_pointed_to already returns nullopt if the idx is invalid
    if (!dir_index_to_refresh.isValid() || !dir_index_opt.has_value()) {
        qCDebug(l_mediasequences) << "No snapshot in memory for this directory. Loading...";
        
        dir_index_to_refresh = 
            append(Types::Directory (
                dir_path
            ));

        dir_index_opt = row_pointed_to (dir_index_to_refresh);
    }

    if (!dir_index_to_refresh.isValid() || !dir_index_opt.has_value()) {

        qCFatal (l_mediasequences) << "Could not create a simple empty directory snapshot on LocalLibrary queue. "
            << "This is a fatal error. Aborting noname.";
    }

    auto prolly_found = pointed_to(dir_index_to_refresh);

    if (
        !prolly_found.has_value()
    ||  !std::holds_alternative<Types::Directory>(prolly_found.value().get())
    ) {
        qCDebug(l_mediasequences) << "Found media item was not a directory.";
        return QtFuture::makeReadyVoidFuture();
    }

    Types::Directory &target_dir = std::get<Types::Directory>(prolly_found.value().get());

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
        return QtFuture::makeReadyVoidFuture();
    }

    qCDebug(l_mediasequences) << "Reading directory: " << target_dir.path;

    // Ask song_factory to extract the metadata of songs, all at once
    return song_factory::batch_extract (not_yet_loaded, {256, true} /* enough for 1440p, can be tweaked any time later */)
        .then(this, [this, dir_index_to_refresh](QList<Types::Song> songs) {
            /*  Re-resolve through the persistent index rather than trusting target_dir from
                above: m_items may have reallocated (another take_snapshot() appending a new
                directory, for instance) during the time batch_extract spent running. */
            auto current_refresh = pointed_to(dir_index_to_refresh);
            if (!current_refresh.has_value()) {
                qCDebug(l_mediasequences) << "Directory was unmapped while metadata extraction was in progress.";
                return;
            }

            Types::Any &current_found = current_refresh.value().get();
            if (!std::holds_alternative<Types::Directory>(current_found)) {
                return;
            }

            Types::Directory &target_dir = std::get<Types::Directory>(current_found);

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

                chosen_cover_provider->register_cover_reference(song.cover);

                if (it != target_dir.songs.end()) {
                    *it = std::move(song);
                } else {
                    target_dir.songs.append(std::move(song));
                }
            }

            emit dataChanged(dir_index_to_refresh, dir_index_to_refresh);
        })
        .onFailed(this, [this] {
            qCWarning(l_mediasequences) << "Batch song load failed for a directory; its songs list was left unchanged.";
        });
}

QFuture<void>
LocalLibrary::take_snapshots (const QStringList &paths)
{
    QList<QFuture<void>> snapshot_futures;

    for (const QString &dir_path : paths) {
        snapshot_futures.append(
            take_snapshot(dir_path)
        );
    }

    return QtFuture::whenAll(snapshot_futures.begin(), snapshot_futures.end());
}

QFuture<void>
LocalLibrary::retake_all_snapshots ()
{
    return take_snapshots(paths())
    .then(this, [this]() {
        emit refreshFinished();
    });
}

QFuture<void>
LocalLibrary::snapshot_known_directories ()
{
    // This is a full refresh.

    auto &manager = configuration::manager::instance();
    using ft = configuration::conf_file_type;

    return take_snapshots(
        manager.read_lines(
            ft::known_music_directories
        )
    ).then(this, [this]() {
        emit refreshFinished();
    });
}

QList<Types::Directory>
LocalLibrary::items () const
{
    return AbstractMediaSequence::items<Types::Directory>();
}

QStringList
LocalLibrary::paths ()
{
    // .path of all items that are Types::Directory
    return sources<Types::Directory>();
}

LocalLibrary::LocalLibrary(QObject *parent)
    : AbstractMediaSequence (parent, container_roles)
{
}