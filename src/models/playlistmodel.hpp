#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QImage>
#include <QUrl>
#include <vector>

struct Song {
    QImage cover;           // Individual song cover (album artwork)
    QString artist;
    QString album;
    QString title;
    qint64 duration;    // Duration in seconds
    QUrl sourcePath;        // Path to audio file
    QUrl coverPath;         // Original path to cover (for reloading)
};

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    // Playlist-level cover (optional, e.g., for playlist thumbnail)
    Q_PROPERTY(QString playlistCover READ playlistCover WRITE setPlaylistCover NOTIFY playlistCoverChanged)

public:
    enum Roles {
        CoverRole = Qt::UserRole + 1,       // Song's individual cover (base64 for QML)
        ArtistRole,
        AlbumRole,
        TitleRole,
        DurationSecsRole,                   // Raw seconds
        SourcePathRole,                     // Audio file path
        CoverPathRole                       // Original cover image path
    };
    Q_ENUM(Roles)

    explicit PlaylistModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Playlist cover (optional, separate from song covers)
    QString playlistCover() const;
    void setPlaylistCover(const QString &base64Image);
    Q_INVOKABLE void setPlaylistCoverFromFile(const QString &filePath);

    // Add song with cover (pass cover as file path, URL, or empty)
    Q_INVOKABLE void appendSong(const QString &title, 
                                const QString &artist, 
                                const QString &album, 
                                qint64 durationSecs,
                                const QString &sourcePath,
                                const QString &coverPath = QString());

    // Update a song's cover individually
    Q_INVOKABLE void setSongCover(int index, const QString &filePath);
    Q_INVOKABLE void setSongCoverFromBase64(int index, const QByteArray &base64Data);

    Q_INVOKABLE void removeSong(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void moveSong(int fromIndex, int toIndex);

    // Getters
    Q_INVOKABLE QString titleAt(int index) const;
    Q_INVOKABLE QString artistAt(int index) const;
    Q_INVOKABLE qint64 durationAt(int index) const;
    Q_INVOKABLE QString coverPathAt(int index) const;  // Returns path, not image data

    Q_INVOKABLE static QString formatDuration(qint64 seconds);

signals:
    void playlistCoverChanged();

private:
    std::vector<Song> m_songs;
    QImage m_playlistCover;  // Optional playlist-level cover
    
    QString imageToBase64Url(const QImage &image) const;
    bool loadImageFromPath(QImage &image, const QString &path);
};