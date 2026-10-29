#pragma once

#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>
#include <QUrl>

#include <cstddef>


// A recipe that tells the cover storage hot cache how to resolve a cover.
// This is a payload factory.
struct CoverRef {
    CoverRef (const QUrl &source_media_path, size_t square_size = 0);

    QUrl source () const; // uri of the original media file
    size_t size () const; // what resolution to load and store this cover in memory and thumbnail cache

    QUrl uri () const; // encoded payload cover_storage will use to resolve and scale the source media's cover

    bool operator==(const CoverRef &) const = default;

    QString encode_base64url () const;
    static CoverRef decode_base64url (const QString &base64url_coverref);

    private:
        // what media file is it
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

        // Pretty printers for QML
        Q_PROPERTY(QString duration_mmss READ duration_mmss)

    public:

        QString  title;
        QString  artist;
        QString  album;
        quint64  duration {0}; // ms
        QUrl     source; // to the audio path

        // ready to print in qml
        QString duration_mmss () const;

        CoverRef cover = CoverRef(source);

        QUrl coveruri () const;

        bool is_valid () const;

        bool operator==(const Song&) const = default; 
    };

    struct Album {
        Q_GADGET

        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString artist MEMBER artist)
        Q_PROPERTY(QList<Types::Song> songs MEMBER songs)
        Q_PROPERTY(quint64 duration READ duration)

    public:

        QString      title;
        QString      artist;
        QList<Song>  songs;

        // sum of all the children
        quint64 duration() const;

        bool operator==(const Album&) const = default; 

        // An empty Album doesn't make it invalid
    };

    struct Playlist {
        Q_GADGET
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QList<Types::Song> songs MEMBER songs)
        Q_PROPERTY(QUrl cover READ firstcover)
        Q_PROPERTY(quint64 duration READ duration)

        Q_PROPERTY(QString duration_mmss READ duration_mmss)

    public:
        QString     title;
        QList<Song> songs;

        // cover of the first element
        QUrl firstcover () const;

        // sum of all the children
        quint64 duration() const;
        QString duration_mmss () const;

        bool operator==(const Playlist&) const = default; 
    };

    struct Directory {
        Q_GADGET
        Q_PROPERTY(QString path MEMBER path)
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QList<Types::Song> songs MEMBER songs)
        
    public:

        Directory () = default;
        explicit Directory (const QString &source_path);

        QString path;
        QString title;

        QList<Song> songs;

        bool is_valid () const;

        bool operator==(const Directory&) const = default; 
    };

    using Any = std::variant<Types::Song, Types::Album, Types::Playlist, Types::Directory>;
}