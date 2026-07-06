#include "lyricsmanifest.hpp"

#include <QQmlEngine>

// Meyers singleton implementation
LyricsManifest &
LyricsManifest::instance()
{
    static LyricsManifest s_instance;
    return s_instance;
}

// qml factory
LyricsManifest *
LyricsManifest::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    
    LyricsManifest *inst = &instance();

    // prevent qml from freeing singleton memory on closure
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}

// private constructor (now doing nothing special)
LyricsManifest::LyricsManifest(QObject *parent)
    : QAbstractListModel(parent)
{
}

int
LyricsManifest::rowCount(const QModelIndex &parent) const
{
    // For list models, parent is always invalid
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_lyrics.size());
}

QVariant
LyricsManifest::data(const QModelIndex &index, int role) const
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

QHash<int, QByteArray>
LyricsManifest::roleNames()
    const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[TextRole] = "text";
    return roles;
}

void
LyricsManifest::appendLyric(unsigned long timestampInMs, const QString &text)
{
    beginInsertRows(QModelIndex(), m_lyrics.size(), m_lyrics.size());
    m_lyrics.push_back({timestampInMs, text});
    endInsertRows();
}

void
LyricsManifest::removeLyric(size_t index)
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_lyrics.erase(m_lyrics.begin() + index);
    endRemoveRows();
}

void
LyricsManifest::clear()
{
    if (m_lyrics.empty())
        return;

    beginResetModel();
    m_lyrics.clear();
    endResetModel();
}

QString
LyricsManifest::textAt(size_t index) const
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return QString();
    return m_lyrics[index].text;
}

unsigned
long LyricsManifest::timestampAt(size_t index)
    const
{
    if (index < 0 || index >= static_cast<int>(m_lyrics.size()))
        return 0;
    return m_lyrics[index].timestampInMs;
}