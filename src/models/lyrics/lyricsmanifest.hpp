#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <vector>

// NOTE: use this as reference to implement new singleton models.

// forward declarations
class QQmlEngine;
class QJSEngine;

struct Lyric {
    unsigned long timestampInMs;
    QString text;
};

// The lyrics of the current playing song, accessible from within the entire player.
class LyricsManifest : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // disable copy and reassignment to guarantee a single instance
    LyricsManifest(const LyricsManifest&) = delete;
    LyricsManifest &operator=(const LyricsManifest&) = delete;

    // singleton instantiation, global access in C++
    static LyricsManifest &instance();
    static LyricsManifest *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // fixed role list
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        TextRole
    };
    Q_ENUM(Roles)

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void appendLyric(unsigned long timestampInMs, const QString &text);
    Q_INVOKABLE void removeLyric(int index);
    Q_INVOKABLE void clear();

    // Get a specific lyric (useful for current time tracking)
    Q_INVOKABLE QString textAt(int index) const;
    Q_INVOKABLE unsigned long timestampAt(int index) const;

private:
    // private constructor
    explicit LyricsManifest(QObject *parent = nullptr);
    
    std::vector<Lyric> m_lyrics;
};