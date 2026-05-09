#pragma once

#include <QAbstractListModel>
#include <QUrl>
#include <QString>
#include <QList>
#include <variant>
#include <vector>

/**
    @brief All the forms of identifiable structures of audio that this player supports.
*/
namespace Types {

    struct Song {
        QString title;
        QString artist;
        QString album;
        qint64  duration; // ms
        QUrl    source; // to the audio path
        QUrl    cover;
    };

    struct Album {
        QString      title;
        QString      artist;
        QList<Song>  songs;
        QUrl         cover;

        qint64 duration() const {
            qint64 total = 0;
            for (const Song &s : songs)
                total += s.duration;
            return total;
        }
    };

    using Playlist = Album;

    // abstract identifier - can mix all types by default
    using Any      = std::variant<Song, Album>;

}

/**
    @brief List model that can contain any type of identifiable audio metadata organization forms, referred as "AudioFormTypes" at the beginning of this file.
    This is not intended to be instantiated directly but rather to use one of the inherited classes.

    The intention is that inherited classes will restrict themselves to only hold 1 or more specified types and update their add(T) and remove(T) functions accordingly.
*/
class AbstractModel : public QAbstractListModel {
    Q_OBJECT

public:

    // Common roles available for all item types
    enum BaseRoles {
        TypeRole  = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        CoverRole,
        DurationRole
    };
    Q_ENUM(BaseRoles)

    // Mirrors the variant index — usable from QML via model.type
    enum ItemType {
        SongType  = 0,  // matches std::variant index of Song
        AlbumType = 1   // matches std::variant index of Album
    };
    Q_ENUM(ItemType)

    explicit AbstractModel(QObject *parent = nullptr);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    // Inherited models call these to manipulate the container
    void append(const Types::Any &item);
    void remove(int index);
    void clearItems();

    // Access to the raw item for inherited classes that need extra roles
    const Types::Any &itemAt(int index) const;
    int itemCount() const;

private:
    std::vector<Types::Any> m_items;
};