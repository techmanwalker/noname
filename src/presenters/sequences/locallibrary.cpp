#include "basicdiskio.hpp"
#include "configuration.hpp"
#include "locallibrary.hpp"
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

QFuture<void>
LocalLibrary::take_snapshot (const QString &dir_path)
{

    auto to_refresh = find(dir_path);

    // this is how we create an empty directory reference
    if (!to_refresh.has_value()) {
        qCDebug(l_mediasequences) << "No snapshot in memory for this directory. Loading...";
        to_refresh = 
            pointed_to (
                append(Types::Directory (
                    dir_path
                ) )
            );
    }

    if (!to_refresh.has_value()) {
        qCFatal (l_mediasequences) << "Could not create a simple empty directory snapshot on LocalLibrary queue. "
            << "This is a fatal error. Aborting noname.";
    }

    Types::Any &found = to_refresh.value().get();

    if (!std::holds_alternative<Types::Directory>(found)) {
        qCDebug(l_mediasequences) << "Found media item was not a directory.";
        return QtFuture::makeReadyVoidFuture();
    }

    Types::Directory &target_dir = std::get<Types::Directory>(found);

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

    /*  Capture 'this' to avoid reevaluating 'find()' safely on the continuation thread, and inject
        the directory path by value to isolate memory context */
    return song_factory::batch_extract (not_yet_loaded, chosen_cover_provider).then (
        [this, dir_path](QList<Types::Song> songs) {
            
            // find again to ensure still exists
            auto current_refresh = find(dir_path);
            if (!current_refresh.has_value()) {
                qCDebug(l_mediasequences) << "Directory was unmapped while metadata extraction was in progress.";
                return;
            }

            Types::Any &current_found = current_refresh.value().get();
            if (!std::holds_alternative<Types::Directory>(current_found)) {
                return;
            }

            Types::Directory &target_dir = std::get<Types::Directory>(current_found);

            // iterate over the already loaded songs
            for (Types::Song &song : songs) {
                // find a song whose .source == path in target_dir.songs
                auto it = std::find_if(target_dir.songs.begin(), target_dir.songs.end(),
                    [&song](const Types::Song &existing_song) {
                        return existing_song.source == song.source;
                    });

                if (it != target_dir.songs.end()) {
                    // if it exists, overwrite its contents with the updated metadata
                    *it = std::move(song);
                } else {
                    // append to the plain list
                    target_dir.songs.append(std::move(song));
                }
            }

            qCDebug(l_mediasequences) << "List synchronized. Total songs in" 
                                      << target_dir.name() << ":" << target_dir.songs.size();

            qCDebug(l_mediasequences) << "Directory cache successfully synchronized for path:" << dir_path;
        }
    );
}

QFuture<void>
LocalLibrary::take_snapshots (const QStringList &paths)
{
    QList<QFuture<void>> refresh_jobs;

    for (const QString &dir_path : paths) {
        refresh_jobs.append(
            take_snapshot (dir_path)
        );
    }

    return QtFuture::whenAll(refresh_jobs.begin(), refresh_jobs.end());
}

QFuture<void>
LocalLibrary::retake_all_snapshots ()
{
    // that currently have a snapshot
    return take_snapshots (paths());
}

QFuture<void>
LocalLibrary::snapshot_known_directories ()
{
    auto &manager = configuration::manager::instance();
    using ft = configuration::conf_file_type;

    return take_snapshots(
        manager.read_lines(
            ft::known_music_directories
        )
    );
}

std::optional<std::reference_wrapper<Types::Any>>
LocalLibrary::find (const QString &path)
{
    // prolly find
    std::optional<std::reference_wrapper<Types::Any>> prolly_ref = 
    AbstractMediaSequence::pointed_to(
        AbstractMediaSequence::find(
            &Types::Directory::path, path
        )
    );

    if (!prolly_ref.has_value()) return std::nullopt;

    Types::Any &ref = prolly_ref.value().get();

    if (std::holds_alternative<Types::Directory>(ref)) {
        return ref;
    }

    return std::nullopt;
}

QList<Types::Directory>
LocalLibrary::items ()
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