#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QUrl>
#include <QObject>
#include <vector>


struct Song {
    QString title;
    QString artist;
    QString album;
    qint64 duration;  // Duration in seconds
    QUrl source;      // Path to audio file
    QUrl cover;       // Original path to cover (for reloading)
};


class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QUrl playlistCover READ playlistCover WRITE setPlaylistCover NOTIFY playlistCoverChanged)

public:
    enum Roles {
        ArtistRole = Qt::UserRole + 1,
        AlbumRole,
        TitleRole,
        DurationSecsRole,  // Raw seconds
        SourceRole,        // Audio file path
        CoverRole          // Song cover image path
    };

    Q_ENUM(Roles)

    explicit PlaylistModel(QObject *parent = nullptr);

    // For list models, parent is always invalid
    int
    rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant
    data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray>
    roleNames() const override;

    // Append and so
    Q_INVOKABLE void append (
        const QString &title,
        const QString &artist,
        const QString &album,
        const qint64   durationSecs,
        const QUrl    &source = QUrl(),
        const QUrl    &cover = QUrl()
    );

    // Remove a song
    Q_INVOKABLE void removeFromPlaylist (
        int index
    );

    // Clear the entire model and also unset playlistCover
    Q_INVOKABLE void clear();

    // Getters
    Q_INVOKABLE QString titleAt(int index) const;
    Q_INVOKABLE QString artistAt(int index) const;
    Q_INVOKABLE qint64  durationAt(int index) const;
    Q_INVOKABLE QUrl    coverAt(int index) const;  // Returns path, not image data

    // For the playlist cover
    QUrl playlistCover() const;
    void setPlaylistCover(const QUrl &cover);

signals:
    void playlistCoverChanged();

private:
    // Song list container
    std::vector<Song> m_songs;

    // I shall be able to fetch this from QML but how?
    QUrl m_playlistCover; // a cover that identifies the playlist itself

};