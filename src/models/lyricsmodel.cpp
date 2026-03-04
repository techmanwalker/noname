#include "lyricsmodel.hpp"

LyricsModel::LyricsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LyricsModel::rowCount(const QModelIndex &parent) const
{
    // For list models, parent is always invalid
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_lyrics.size());
}

QVariant LyricsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_lyrics.size()))
        return QVariant();

    const Lyric &lyric = m_lyrics[index.row()];

    switch (role) {
    case TimestampRole:
        return QVariant::fromValue(lyric.timestampInMs);
    case TextRole:
        return lyric.text;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LyricsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[TextRole] = "text";
    return roles;
}

void LyricsModel::appendLyric(unsigned long timestampInMs, const QString &text)
{
    beginInsertRows(QModelIndex(), m_lyrics.size(), m_lyrics.size());
    m_lyrics.push_back({timestampInMs, text});
    endInsertRows();
}

void LyricsModel::removeLyric(int index)
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_lyrics.erase(m_lyrics.begin() + index);
    endRemoveRows();
}

void LyricsModel::clear()
{
    if (m_lyrics.empty())
        return;

    beginResetModel();
    m_lyrics.clear();
    endResetModel();
}

QString LyricsModel::textAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return QString();
    return m_lyrics[index].text;
}

unsigned long LyricsModel::timestampAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return 0;
    return m_lyrics[index].timestampInMs;
}