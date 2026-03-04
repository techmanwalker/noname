#include "playlistmodel.hpp"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_songs.size());
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_songs.size()))
        return QVariant();

    const Song &song = m_songs[index.row()];

    switch (role) {
    case CoverRole:
        return imageToBase64Url(song.cover);  // Each song has its own cover
    case ArtistRole:
        return song.artist;
    case AlbumRole:
        return song.album;
    case TitleRole:
        return song.title;
    case DurationSecsRole:
        return QVariant::fromValue(song.duration);
    case SourcePathRole:
        return song.sourcePath;
    case CoverPathRole:
        return song.coverPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CoverRole] = "cover";              // Song's individual cover image
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[TitleRole] = "title";
    roles[DurationSecsRole] = "duration";
    roles[SourcePathRole] = "sourcePath";
    roles[CoverPathRole] = "coverPath";      // Path to cover file
    return roles;
}

// Playlist cover (optional, separate from song covers)
QString PlaylistModel::playlistCover() const
{
    return imageToBase64Url(m_playlistCover);
}

void PlaylistModel::setPlaylistCover(const QString &base64Url)
{
    if (base64Url.startsWith("data:image")) {
        QString base64Data = base64Url.section(',', 1);
        QByteArray data = QByteArray::fromBase64(base64Data.toUtf8());
        m_playlistCover.loadFromData(data);
    }
    emit playlistCoverChanged();
}

void PlaylistModel::setPlaylistCoverFromFile(const QString &filePath)
{
    m_playlistCover.load(filePath);
    emit playlistCoverChanged();
}

// Add song - coverPath can be local file path, qrc:/ path, or http URL
void PlaylistModel::appendSong(const QString &title, 
                               const QString &artist, 
                               const QString &album, 
                               qint64 duration,
                               const QString &sourcePath,
                               const QString &coverPath)
{
    beginInsertRows(QModelIndex(), m_songs.size(), m_songs.size());
    
    Song song;
    song.title = title;
    song.artist = artist;
    song.album = album;
    song.duration = duration;
    song.sourcePath = QUrl::fromUserInput(sourcePath);
    song.coverPath = QUrl::fromUserInput(coverPath);
    
    // Try to load cover if path provided
    if (!coverPath.isEmpty()) {
        loadImageFromPath(song.cover, coverPath);
    }
    
    m_songs.push_back(song);
    endInsertRows();
}

bool PlaylistModel::loadImageFromPath(QImage &image, const QString &path)
{
    if (path.startsWith("qrc:/")) {
        // Resource file
        return image.load(path.mid(3));  // Remove "qrc:" prefix
    } else if (path.startsWith("http://") || path.startsWith("https://")) {
        // For HTTP, you'd need async loading ( QNetworkAccessManager )
        // For now, return false - implement async if needed
        return false;
    } else if (path.startsWith("data:image")) {
        // Base64 data URL
        QString base64Data = path.section(',', 1);
        QByteArray data = QByteArray::fromBase64(base64Data.toUtf8());
        return image.loadFromData(data);
    } else {
        // Local file path
        return image.load(path);
    }
}

void PlaylistModel::setSongCover(int index, const QString &filePath)
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return;
    
    if (loadImageFromPath(m_songs[index].cover, filePath)) {
        m_songs[index].coverPath = filePath;
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {CoverRole, CoverPathRole});
    }
}

void PlaylistModel::setSongCoverFromBase64(int index, const QByteArray &base64Data)
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return;
    
    if (m_songs[index].cover.loadFromData(QByteArray::fromBase64(base64Data))) {
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), {CoverRole});
    }
}

void PlaylistModel::removeSong(int index)
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_songs.erase(m_songs.begin() + index);
    endRemoveRows();
}

void PlaylistModel::clear()
{
    if (m_songs.empty())
        return;

    beginResetModel();
    m_songs.clear();
    endResetModel();
}

void PlaylistModel::moveSong(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_songs.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_songs.size()))
        return;
    
    int destRow = toIndex > fromIndex ? toIndex + 1 : toIndex;
    if (beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), destRow)) {
        std::swap(m_songs[fromIndex], m_songs[toIndex]);
        endMoveRows();
    }
}

QString PlaylistModel::titleAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return QString();
    return m_songs[index].title;
}

QString PlaylistModel::artistAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return QString();
    return m_songs[index].artist;
}

qint64 PlaylistModel::durationAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return 0;
    return m_songs[index].duration;
}

QString PlaylistModel::coverPathAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return QString();
    return m_songs[index].coverPath.toString();
}

QString PlaylistModel::formatDuration(qint64 seconds)
{
    if (seconds < 0) return "--:--";
    
    qint64 hours = seconds / 3600;
    qint64 mins = (seconds % 3600) / 60;
    qint64 secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

QString PlaylistModel::imageToBase64Url(const QImage &image) const
{
    if (image.isNull())
        return QString();
    
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    
    return QString("data:image/png;base64,") + byteArray.toBase64();
}