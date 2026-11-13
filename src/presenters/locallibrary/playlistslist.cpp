#include "basicdiskio.hpp"
#include "virtual/manager.hpp"
#include "defaultroles.hpp"
#include "coveruris.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp" // IWYU pragma: keep
#include "standardpaths.hpp"
#include "playlistslist.hpp"
#include "l_locallibrary.hpp"

#include <QtConcurrent/QtConcurrent>

PlaylistsListLI::PlaylistsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager)
    : AbstractMediaSequence(parent, container_roles),
      cm(std::move(confmanager))
{}

QDir
PlaylistsListLI::directory()
{
    return QDir(standardpaths::dir(standardpaths::standard_dirs::playlists));
}

QList<Types::Playlist>
PlaylistsListLI::items () const
{
    return BasicSequenceOps<PlaylistsListLI, Types::Playlist>::items();
}

QString
PlaylistsListLI::file_path_by_name(const QString &name)
{
    return directory().filePath(name);
}

bool
PlaylistsListLI::pointed_to(const QPersistentModelIndex &idx, std::function<bool (const Types::Playlist &)> callback)
{
    return AbstractMediaSequence::pointed_to<Types::Playlist>(idx, callback);
}

QFuture<void>
PlaylistsListLI::refresh ()
{
    clear();
    const quint64 generation = ++m_refresh_generation;

    // Every text file containing one path by line counts as a playlist
    QStringList playlist_files = diskio::list_dir(
        directory().path(), 
        true
    );

    QList<QFuture<void>> pending;
    pending.reserve(playlist_files.size());

    for (const QString &playlist_text : playlist_files) {
        const QStringList song_paths = diskio::read_lines(playlist_text).value_or({});

        pending.append(song_factory::batch_extract(
            song_paths, 
            {
                covers::default_cover_thumbnail_size, 
                true
            }
        ).then(this, [playlist_text, generation, this](QList<Types::Song> songs_of_playlist) {
            if (generation != m_refresh_generation) return; // a newer refresh superseded this one

            // song_factory reads the metadata for each path
            Types::Playlist playlist {
                QFileInfo(playlist_text).baseName(),
                std::move(songs_of_playlist),
            };

            // load into memory
            append(std::move(playlist));
        }));
    }

    // Finishes once every continuation above has run (or been cancelled).
    return QtFuture::whenAll(pending.begin(), pending.end());
}

QFuture<bool>
PlaylistsListLI::save_playlist_to_disk(const QString &name, QList<QUrl> sources)
{
    QStringList source_lines;

    for (QUrl source: sources) {
        // treat paths with no schema as local files
        source_lines.append(
            source.isLocalFile() ?
                source.toLocalFile() :
                source.toString()
        );
    }

    // commit the write to disk, wait for the future as needed
    return diskio::write_lines(
        file_path_by_name(name),
        source_lines
    ).then(this, [this](bool success) {
        refresh();
        return success;
    });
}

QFuture<bool>
PlaylistsListLI::delete_playlists (const QStringList &names)
{
    return QtConcurrent::run([names]() -> bool {
            bool all_succeeded { true };

            for (const QString &name : names) {
                if (!diskio::move_to_trash(file_path_by_name(name))) {
                    qCDebug(l_locallibrary) << "Deleting playlist" << name << "failed."; 
                    all_succeeded = false;
                }
            }

            return all_succeeded;
        }
    ).then(this, [this](bool all_succeeded) {
        refresh();
        return all_succeeded;
    });
}
