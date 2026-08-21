#pragma once

#include <QAbstractListModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QObject>
#include <memory>
#include <string>
#include <vector>

#include <QtQmlIntegration/qqmlintegration.h>

#include "lyricsmanifest-in.hpp"

Q_DECLARE_LOGGING_CATEGORY(l_lyricsmanifest);

// forward declarations
class QQmlEngine;
class QJSEngine;

class LyricsManifestPrivate;

// The lyrics of the current playing song, accessible from within the entire player.
class LyricsManifestLI : public QAbstractListModel, public LyricsManifest
{
    Q_OBJECT
    Q_INTERFACES(LyricsManifest)

public:
    explicit LyricsManifestLI(QObject *parent);
    ~LyricsManifestLI() override; // explicitly required for std::unique_ptr with incomplete types

    QFuture<void> repopulate_with_lyrics_for_file(const QString &source) override;

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    std::vector<std::string> current_lines() const;

    // Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void clear() override;

signals:
    void linesChanged();

private:
    std::unique_ptr<LyricsManifestPrivate> m_d;
};