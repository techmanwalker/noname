#include "lyricsmanifest.hpp"

#include "lines.hpp"
#include "metadata.hpp" // from syrinc
#include "process.hpp"
#include "tokens.hpp"

#include <QFile>
#include <QQmlEngine>

#include <QtConcurrent/QtConcurrent>
#include <qloggingcategory.h>
#include <qreadwritelock.h>

Q_LOGGING_CATEGORY(l_lyricsmanifest, "noname.lyrics");

using namespace syrinc::audio;
using namespace syrinc::timestamps;

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
    : QAbstractListModel(parent),
      m_roles(lyrics_roles)
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

/// Retrieves the value of a specific role for the item at the given index
QVariant
LyricsManifest::data(
    const QModelIndex &index,
    int role
) const
{
    QReadLocker locker (&m_lock);

    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_lyrics.size()))
        return {};

    return m_roles.extract(role, m_lyrics[index.row()]);
}

/// Returns the role name map, allowing QML to resolve properties by their string names.
QHash<int, QByteArray>
LyricsManifest::roleNames() const
{
    return m_roles.roleNames();
}

std::vector<lyric>
LyricsManifest::current_lines() const
{
    return m_lyrics;
}

// left open to also read stray .lrc files in the future
QFuture<void>
LyricsManifest::repopulate_with_lyrics_for_file (const QString &source)
{
    // read lyrics from the LYRICS tag or a stray .lrc file and
    // repopulate this model
    // here they already come unwrapped
    return read_from_metadata_tag(source).then(
        // do not execute in the model context just yet
        [] (filelines lines) {
            std::vector<lyric> decoupled_lines;

            for (const std::string &line : lines) {
                // unwrap must ensure each line has exactly 1 timestamp
                std::vector<timestamp> prolly_single_timestamp = lines::line_timestamps(line);

                timestamp timestamp_to_use;

                if (prolly_single_timestamp.size() != 1) [[unlikely]] {
                    qCWarning(l_lyricsmanifest) << "The syrinc lyrics submodule did not unwrap the lyrics correctly. "
                        << "This should not happen, and lyrics progression may not work as expected.";

                    if (prolly_single_timestamp.size() == 0) {
                        timestamp_to_use = timestamp(0);
                    } else {
                        timestamp_to_use = prolly_single_timestamp.at(0);
                    }
                } else [[likely]] {
                    timestamp_to_use = prolly_single_timestamp.at(0);
                }

                // successfully separated t
                decoupled_lines.emplace_back(
                    timestamp_to_use,
                    QString::fromStdString(tokens::trim_string(lines::strip_timestamps(line)))
                );
            }

            return std::move(decoupled_lines);
        }
    ).then(this, [this](std::vector<lyric> lines_to_add) {
        beginResetModel();

        {
            QWriteLocker locker (&m_lock);
            m_lyrics = std::move(lines_to_add);
        }

        endResetModel();
    });
}

QFuture<filelines>
LyricsManifest::read_from_metadata_tag (const QString &source)
{
    if (!QFile::exists(source)) {
        return QtFuture::makeReadyValueFuture(filelines());
    }
    
    // Create a local lvalue string
    return QtConcurrent::run([source] () {
        std::string nativeString = source.toStdString();
        return get_audio_lyrics(nativeString);
    }).then([] (filelines extracted_lines) {
        // do not explicitly correct offset, only do unwrap
        return process::process_lyrics(extracted_lines, {
            .dropmetadata=true,
            .unwrap=true,
        });
    });
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