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
#include "audioengine-in.hpp"

Q_DECLARE_LOGGING_CATEGORY(l_lyricsmanifest);

class LyricsManifestPrivate;

// The lyrics of the current playing song, accessible from within the entire player.
class LyricsManifestLI : public QAbstractListModel, public LyricsManifest
{
    Q_OBJECT
    Q_INTERFACES(LyricsManifest)

public:
    explicit LyricsManifestLI(QObject *parent, std::shared_ptr<audio_engine> position_tracker);
    ~LyricsManifestLI() override; // explicitly required for std::unique_ptr with incomplete types

    QFuture<void> repopulate_with_lyrics_for_file(const QString &source) override;

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    std::vector<std::string> current_lines() const;

    std::optional<quint64> ts_of_lyric_at(quint64 ts_ms) const override;
    std::optional<quint64> next_lyric_ts_at(quint64 ts_ms) const override;
    std::optional<QString> lyric_at(quint64 ts_ms) const override;

    // Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void clear() override;

signals:
    void linesChanged();

public slots:
    QFuture<void> load_current_track_lyrics ();

    // Fire this (e.g. from the timer that polls for duration) to reconcile
    // the highlighted lyric against the current playback position.
    void poll_highlighted_line_change ();

private:
    std::unique_ptr<LyricsManifestPrivate> m_d;

    std::shared_ptr<audio_engine> ae;
};