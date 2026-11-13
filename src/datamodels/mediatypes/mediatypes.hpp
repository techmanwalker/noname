#pragma once

#include <QFileInfo>
#include <QLoggingCategory>
#include <QSize>
#include <QString>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(l_mediatypes)

namespace Types {
    struct Song {
        Q_GADGET

        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString artist MEMBER artist)
        Q_PROPERTY(QString album MEMBER album)
        Q_PROPERTY(quint64 duration MEMBER duration)
        Q_PROPERTY(QUrl source MEMBER source)
        Q_PROPERTY(QUrl cover READ coveruri)

        Q_PROPERTY(QString fileformat MEMBER fileformat)
        Q_PROPERTY(QString genre MEMBER genre)
        Q_PROPERTY(uint year MEMBER year)
        Q_PROPERTY(int channels MEMBER channels)
        Q_PROPERTY(int sample_rate MEMBER sample_rate)
        Q_PROPERTY(int bitrate_kbps MEMBER bitrate_kbps)

        // Pretty printers for QML
        Q_PROPERTY(QString duration_mmss READ duration_mmss)

    public:

        QString  title;
        QString  artist;
        QString  album;
        quint64  duration {0}; // ms
        QUrl     source; // to the audio path

        // Extra metadata, intended to only show on ui when non empty
        QString fileformat;
        QString genre;
        uint    year {0}; // taglib does not support bce time :/
        int     channels {0};
        int     sample_rate {0};
        int     bitrate_kbps {0};

        // ready to print in qml
        QString duration_mmss () const;

        QByteArray source_base64 () const;
        QUrl coveruri () const;

        bool is_valid () const;

        bool operator==(const Song&) const = default; 
    };
}

struct base_media_container_ {
    Q_GADGET

    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QList<Types::Song> songs MEMBER songs)
    Q_PROPERTY(QUrl cover READ firstcover)
    Q_PROPERTY(quint64 duration READ duration)

    Q_PROPERTY(QString duration_mmss READ duration_mmss)

public:
    QString            title;
    QList<Types::Song> songs;

    // cover of the first element
    QUrl firstcover () const;

    // sum of all the children
    quint64 duration() const;
    QString duration_mmss () const;

    bool operator==(const base_media_container_&) const = default;
};

/// All the forms of identifiable structures of audio that this player supports.
namespace Types {

    struct Album     : public base_media_container_{
        Q_GADGET
        Q_PROPERTY(QString artist MEMBER artist)
    public:
        QString artist;
    };

    struct Playlist  : public base_media_container_ {
        Q_GADGET
    };

    struct Directory : public base_media_container_ {
        Q_GADGET
        Q_PROPERTY(QString path MEMBER path)
        
    public:
        QString path;
        bool is_valid () const;
    };

    using Any = std::variant<Types::Song, Types::Album, Types::Playlist, Types::Directory>;
}

template <typename T>
concept media_type =
    std::convertible_to<std::remove_cvref_t<T>, Types::Any>;

// shortcuts
// the bytes behind a base64url string, or nothing if it is not exactly what the encoder makes
std::optional<QByteArray>
decodeBase64Url (const QByteArray &base64url);

QByteArray
toBase64Url (const QUrl &uri);

QUrl
fromBase64Url (const QByteArray &base64url);

bool
isBase64Url (const QByteArray &s);
