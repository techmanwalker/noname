#include "playlistmodel.hpp"

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    // Plain lists don't have children, so if parent is valid, return 0
    if (parent.isValid())
        return 0;
    
    return static_cast<int>(m_songs.size());
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_songs.size()))
        return QVariant();

    const Song &song = m_songs[index.row()];

    // Map roles to structure data
        switch (role) {
        case ArtistRole:       return song.artist;
        case AlbumRole:        return song.album;
        case TitleRole:        return song.title;
        case DurationSecsRole: return song.duration;
        case SourceRole:       return song.source;
        case CoverRole:        return song.cover;

        default:               return QVariant();
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    // These are the EXACT names to use in QML (such as model.artist.)
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[DurationSecsRole] = "duration";
    roles[SourceRole] = "source";
    roles[CoverRole] = "cover";
    return roles;
}

void PlaylistModel::append(const QString &title, const QString &artist, const QString &album, const qint64 durationSecs, const QUrl &source, const QUrl &cover)
{
    // Notify QML that we'll insert a row at the end
    int newRow = static_cast<int>(m_songs.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    
    m_songs.push_back({title, artist, album, durationSecs, source, cover});
    
    endInsertRows();
}

void PlaylistModel::removeFromPlaylist(int index)
{
    if (index < 0 || index >= static_cast<int>(m_songs.size()))
        return;

    // Notify QML that we'll remove the row at "index"
    beginRemoveRows(QModelIndex(), index, index);
    
    m_songs.erase(m_songs.begin() + index);
    
    endRemoveRows();
}

void PlaylistModel::clear()
{
    // ResetModel completely removes the view in QML and forces to redraw it
    beginResetModel();
    m_songs.clear();
    endResetModel();

    // Also clear the playlist cover
    setPlaylistCover(QUrl()); 
}

// --- Q_INVOKABLE methods (Getters) ---

QString PlaylistModel::titleAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size())) return QString();
    return m_songs[index].title;
}

QString PlaylistModel::artistAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size())) return QString();
    return m_songs[index].artist;
}

qint64 PlaylistModel::durationAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size())) return 0;
    return m_songs[index].duration;
}

QUrl PlaylistModel::coverAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_songs.size())) return QUrl();
    return m_songs[index].cover; 
}

// --- Q_PROPERTY implementation ---

QUrl PlaylistModel::playlistCover() const
{
    return m_playlistCover;
}

void PlaylistModel::setPlaylistCover(const QUrl &cover)
{
    // Only emit the signal if it actually changed
    if (m_playlistCover != cover) {
        m_playlistCover = cover;
        emit playlistCoverChanged();
    }
}