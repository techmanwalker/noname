#pragma once

#include <QString>
#include <QUrl>

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

    using Any = std::variant<Types::Song, Types::Album>;
}