#pragma once

#include <QFileInfo>
#include <QString>
#include <QUrl>

#include <QtQmlIntegration/qqmlintegration.h>

/// All the forms of identifiable structures of audio that this player supports.
namespace Types {

    struct Song {
        Q_GADGET
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString artist MEMBER artist)
        Q_PROPERTY(QString album MEMBER album)
        Q_PROPERTY(quint64 duration MEMBER duration)
        Q_PROPERTY(QUrl source MEMBER source)
        Q_PROPERTY(QUrl cover MEMBER cover)
    
    public:

        QString title;
        QString artist;
        QString album;
        quint64 duration; // ms
        QUrl    source; // to the audio path
        QUrl    cover;

        bool is_valid () const;

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

        // An empty Playlist doesn't make it invalid

        bool operator==(const Album&) const = default;
    };

    using Playlist = Album;

    struct Directory {
        explicit Directory (const QString &source_path);

        QString path;
        QString title;

        QList<Song> songs;

        bool is_valid () const;
    };

    using Any = std::variant<Types::Song, Types::Album, Types::Directory>;
}

// Expose the structures and their sequences at compile-time
Q_DECLARE_METATYPE(Types::Song)
Q_DECLARE_METATYPE(QList<Types::Song>)