#pragma once

#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>
#include <QUrl>

#include <QtQmlIntegration/qqmlintegration.h>
#include <cstddef>

/*  Deterministic, session-independent cache key for a song's thumbnail.
    Hashing the absolute path (not file contents) keeps this cheap — an MD5
    over a path-length string is microseconds, dwarfed by the decode/encode
    it lets us skip. crop_and_resize is folded in too, since the cache
    stores the already-resized image: without it, two call sites requesting
    different sizes for the same song would collide on one cache entry and
    silently serve the wrong resolution to whichever asked second. */
struct CoverRef {
    CoverRef (const QUrl &source_media_path, size_t square_size = 0);

    QUrl source () const;
    QString hash () const;
    size_t size () const;

    // path on disk where the thumbnail is stored and retrieved
    QString thumbnail_file_path () const;

    // whether the file in disk actually exists
    bool thumbnail_file_exists () const;

    QUrl uri () const;

    bool operator==(const CoverRef &) const = default;

    private:
        QUrl m_source;

        // a square size of 0 means full resolution
        size_t m_square_size;
};

Q_DECLARE_LOGGING_CATEGORY(l_mediatypes)

/// All the forms of identifiable structures of audio that this player supports.
namespace Types {

    struct Song {
        Q_GADGET
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString artist MEMBER artist)
        Q_PROPERTY(QString album MEMBER album)
        Q_PROPERTY(quint64 duration MEMBER duration)
        Q_PROPERTY(QUrl source MEMBER source)
        Q_PROPERTY(QUrl cover READ coveruri)

    private:
       
    
    public:

        QString  title;
        QString  artist;
        QString  album;
        quint64  duration; // ms
        QUrl     source; // to the audio path

        CoverRef cover = CoverRef(source);

        QUrl coveruri () const;

        bool is_valid () const;
    };

    struct Album {
        QString      title;
        QString      artist;
        QList<Song>  songs;

        // sum of all the children
        quint64 duration() const;

        // An empty Playlist doesn't make it invalid
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
Q_DECLARE_METATYPE(CoverRef)