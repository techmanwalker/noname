#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <vector>

struct Lyric {
    unsigned long timestampInMs;
    QString text;
};

class LyricsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        TextRole
    };
    Q_ENUM(Roles)

    explicit LyricsModel(QObject *parent = nullptr);

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
    std::vector<Lyric> m_lyrics;
};