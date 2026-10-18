#include "abstractmediasequence.hpp"
#include "basicdiskio.hpp"
#include "manager-in.hpp"
#include "defaultroles.hpp"
#include "coveruris.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp" // IWYU pragma: keep
#include "standardpaths.hpp"
#include "playlistslist.hpp"

PlaylistsListLI::PlaylistsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager)
    : AbstractMediaSequence(parent, container_roles),
      cm(confmanager)
{}

QFuture<void>
PlaylistsListLI::read_conf_and_load ()
{
    using configuration::conf_file_type::shortcuts;

    // Every text file containing one path by line counts as a playlist
    QStringList playlist_files = diskio::list_dir(
        dir(standardpaths::standard_dirs::playlists), 
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
                true}
        ).then(this, [playlist_text, this](QList<Types::Song> songs_of_playlist) {

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
    return QtFuture::whenAll(pending.begin(), pending.end())
        .then(this, [](const QList<QFuture<void>> &) {});
}

// a shortcut could be actually anything, just not now
void PlaylistsListLI::append(const Types::Playlist &playlist)            { AbstractMediaSequence::append(playlist); }
void PlaylistsListLI::remove(size_t index)                               { AbstractMediaSequence::remove(index); }
void PlaylistsListLI::clear()                                            { AbstractMediaSequence::clear(); }