#pragma once

// from syrinc
#include "globals.hpp" // Note: this is globals.hpp from syrinc
#include "rolecompiler.hpp"
#include "timestamps.hpp"

#include <QAbstractListModel>
#include <QFuture>
#include <QLoggingCategory>
#include <QObject>
#include <QReadWriteLock>

#include <QtQmlIntegration/qqmlintegration.h>
#include <qloggingcategory.h>
#include <qreadwritelock.h>

Q_DECLARE_LOGGING_CATEGORY(l_lyricsmanifest);

using namespace syrinc;

// decoupled
struct lyric {
    syrinc::timestamps::timestamp ts;
    QString text;
};

static const RoleDefinitions<lyric> lyrics_roles = {
    { "timestamp", [](const lyric &x) -> QVariant {
        return static_cast<qulonglong>(x.ts.as_ms());
    }},
    { "text", [](const lyric &x) -> QVariant {
        return x.text;
    }}
};

// forward declarations
class QQmlEngine;
class QJSEngine;

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

    QFuture<void> repopulate_with_lyrics_for_file (const QString &source);

    // Required QAbstractListModel implementations
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    std::vector<lyric> current_lines() const;

    // Q_INVOKABLE makes them callable from QML
    Q_INVOKABLE void clear();

signals:
    void linesChanged();

private:
    // private constructor
    explicit LyricsManifest(QObject *parent = nullptr);

    // tell syrinc to read the audio LYRICS tag to fetch the lyrics
    [[nodiscard ("Unused file read execution result")]] QFuture<filelines> read_from_metadata_tag (const QString &source);

    
    // currently displayed lyrics
    std::vector<lyric> m_lyrics;

    // read roles
    CompiledRoleSet<lyric> m_roles;

    mutable QReadWriteLock m_lock;
};