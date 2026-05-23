#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <vector>

// forward declarations
class QQmlEngine;
class QJSEngine;

struct Lyric {
    unsigned long timestampInMs;
    QString text;
};

class LyricsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // disable copy and reassignment to guarantee a single instance
    LyricsModel(const LyricsModel&) = delete;
    LyricsModel& operator=(const LyricsModel&) = delete;

    // singleton instantiation, global access in C++
    static LyricsModel& instance();
    static LyricsModel* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        TextRole
    };
    Q_ENUM(Roles)

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Your custom methods - Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void appendLyric(unsigned long timestampInMs, const QString &text);
    Q_INVOKABLE void removeLyric(int index);
    Q_INVOKABLE void clear();

    // Optional: get a specific lyric (useful for current time tracking)
    Q_INVOKABLE QString textAt(int index) const;
    Q_INVOKABLE unsigned long timestampAt(int index) const;

private:
    // private constructor
    explicit LyricsModel(QObject *parent = nullptr);
    
    std::vector<Lyric> m_lyrics;
};