#include "abstractmediasequence.hpp" // for l_mediasequences
#include "basicdiskio.hpp"
#include "coverprovider.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"

#include <QFileInfo>
#include <QFuture>
#include <QHash>
#include <QString>
#include <QStringList>

namespace Types {

class Directory {

public:
    explicit Directory(const QString &path, std::shared_ptr<cover_provider> coverprovider)
        : m_path(path),
          chosen_cover_provider(std::move(coverprovider)) // C++ idiom: move shared_ptr
    {
    }

    ~Directory() = default;

    QString m_path;

    QFuture<QList<Types::Song>> songs() {
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

    void refresh_cache() {
        m_children_file_paths_cache = diskio::list_dir(m_path, true);
    }

    QStringList children_paths () const {
        return m_children_file_paths_cache;
    }

    QString name() const {
        return QFileInfo(m_path).fileName();
    }

    QString path() const {
        return m_path;
    }

    // where are covers cached?
    std::shared_ptr<cover_provider> chosen_cover_provider;

private:
    QHash<QString, Types::Song> m_song_cache;
    QStringList m_children_file_paths_cache;
};

}