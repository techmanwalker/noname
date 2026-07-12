#include "directory.hpp"
#include <qobject.h>

namespace Types {

Directory::Directory(const QString &path, std::shared_ptr<cover_provider> coverprovider)
    : m_path(path),
        chosen_cover_provider(std::move(coverprovider)) // C++ idiom: move shared_ptr
{
}

QFuture<QList<Types::Song>> 
Directory::songs() 
{
    qCInfo (l_mediasequences) << "Started printing songs for directory " << m_path;

    QList<QFuture<Types::Song>> songs_futures;
    songs_futures.reserve(m_children_file_paths_cache.size());

    for (const QString &source : std::as_const(m_children_file_paths_cache)) {
        // efficient lookup in cache
        auto it = m_song_cache.find(source);
        
        if (it != m_song_cache.end()) {
            // Qt6: makeReadyValueFuture replaces old methods
            songs_futures.append(QtFuture::makeReadyValueFuture(it.value()));
        } else {
            songs_futures.append(song_factory::extract(QUrl::fromLocalFile(source), chosen_cover_provider));
        }

        qCInfo (l_mediasequences) << "Queued song " << source << " for metadata loading.";
    }

    qCInfo (l_mediasequences) << "All songs queued. Waiting for all loads to finish.";

    return QtFuture::whenAll(songs_futures.begin(), songs_futures.end())
        .then([this](QList<QFuture<Types::Song>> resolved_songs) {

            qCInfo (l_mediasequences) << "Finished metadata load. Started to append";

            QList<Types::Song> songs_out;
            songs_out.reserve(resolved_songs.size());

            for (QFuture<Types::Song> &song_future : resolved_songs) {
                // Qt6: takeResult() moves the value, avoiding unnecessary copies
                Types::Song song = song_future.takeResult();

                qCInfo (l_mediasequences) << "Appending song \"" << song.title << "\"...";
                
                // Update cache for future calls
                m_song_cache.insert(song.source.toLocalFile(), song);
                
                songs_out.append(std::move(song));

                qCInfo (l_mediasequences) << "Song \"" << song.title << "\" successfully appended.";
            }

            return songs_out;
        });
}

void 
Directory::refresh_cache ()
{
    m_children_file_paths_cache = diskio::list_dir(m_path, true);
}

QStringList
Directory::children_paths () const
{
    return m_children_file_paths_cache;
}

QString 
Directory::name() const
{
    return QFileInfo(m_path).fileName();
}

QString 
Directory::path() const 
{
    return m_path;
}

}