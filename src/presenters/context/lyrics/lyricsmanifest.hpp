#pragma once

#include <QAbstractListModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QObject>
#include <memory>
#include <string>
#include <vector>

#include <QtQmlIntegration/qqmlintegration.h>

Q_DECLARE_LOGGING_CATEGORY(l_lyricsmanifest);

// forward declarations
class QQmlEngine;
class QJSEngine;

class LyricsManifestPrivate;

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

    QFuture<void> repopulate_with_lyrics_for_file(const QString &source);

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    std::vector<std::string> current_lines() const;

    // Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void clear();

signals:
    void linesChanged();

private:
    // private constructor
    explicit LyricsManifest(QObject *parent = nullptr);
    ~LyricsManifest() override; // explicitly required for std::unique_ptr with incomplete types

    std::unique_ptr<LyricsManifestPrivate> m_d;
};