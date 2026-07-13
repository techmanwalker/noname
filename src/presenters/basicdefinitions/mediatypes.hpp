#pragma once

#include <QFileInfo>
#include <QString>
#include <QUrl>

#include <QtQmlIntegration/qqmlintegration.h>

/// All the forms of identifiable structures of audio that this player supports.
namespace Types {

    struct Song {
        QString title;
        QString artist;
        QString album;
        quint64 duration; // ms
        QUrl    source; // to the audio path
        QUrl    cover;

        // why would I even need to specify this? this is outright insane
        // but .find won't even compile without it
        bool operator==(const Song&) const = default;
    };

    struct Album {
        QString      title;
        QString      artist;
        QList<Song>  songs;
        QUrl         cover;

        // sum of all the children
        quint64 duration() const {
            quint64 total = 0;
            for (const Song &s : songs)
                total += s.duration;
            return total;
        }

        bool operator==(const Album&) const = default;
    };

    using Playlist = Album;

    struct Directory {
        QString path;
        QString name () const {
            return QFileInfo(path).fileName();
        }

        QList<Song> songs;
    };

    using Any = std::variant<Types::Song, Types::Album, Types::Directory>;
}

// Expose the structures and their sequences at compile-time
Q_DECLARE_METATYPE(Types::Song)
Q_DECLARE_METATYPE(QList<Types::Song>)

class QmlSongRegistration 
{
    Q_GADGET
    QML_FOREIGN(Types::Song)
    QML_NAMED_ELEMENT(Song)
};