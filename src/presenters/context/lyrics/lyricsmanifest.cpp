#include "lyricsmanifest.hpp"

// syrinc includes now strictly bound to the implementation file
#include "globals.hpp" 
#include "rolecompiler.hpp"
#include "timestamps.hpp"
#include "lines.hpp"
#include "metadata.hpp"
#include "process.hpp"
#include "tokens.hpp"

#include <QFile>
#include <QQmlEngine>
#include <QtConcurrent/QtConcurrent>
#include <qloggingcategory.h>
#include <qreadwritelock.h>

Q_LOGGING_CATEGORY(l_lyricsmanifest, "noname.lyrics");

using namespace syrinc;
using namespace syrinc::audio;
using namespace syrinc::timestamps;

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

// Private implementation class sealing the internal components
class LyricsManifestPrivate {
public:
    LyricsManifestPrivate() : m_roles(lyrics_roles) {}

    std::vector<lyric> m_lyrics;
    CompiledRoleSet<lyric> m_roles;
    mutable QReadWriteLock m_lock;

    QFuture<filelines> read_from_metadata_tag(const QString &source) {
        if (!QFile::exists(source)) {
            return QtFuture::makeReadyValueFuture(filelines());
        }
        
        return QtConcurrent::run([source]() {
            std::string nativeString = source.toStdString();
            return get_audio_lyrics(nativeString);
        }).then([](filelines extracted_lines) {
            return process::process_lyrics(extracted_lines, {
                .dropmetadata=true,
                .unwrap=true,
            });
        });
    }
};

// private constructor utilizing unique_ptr standard setup
LyricsManifestLI::LyricsManifestLI(QObject *parent)
    : QAbstractListModel(parent),
      m_d(std::make_unique<LyricsManifestPrivate>())
{
}

// explicit definition handles incomplete types gracefully across object borders
LyricsManifestLI::~LyricsManifestLI() = default;

int
LyricsManifestLI::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_d->m_lyrics.size());
}

QVariant
LyricsManifestLI::data(const QModelIndex &index, int role) const
{
    QReadLocker locker(&m_d->m_lock);

    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_d->m_lyrics.size()))
        return {};

    return m_d->m_roles.extract(role, m_d->m_lyrics[index.row()]);
}

QHash<int, QByteArray>
LyricsManifestLI::roleNames() const
{
    return m_d->m_roles.roleNames();
}

std::vector<std::string>
LyricsManifestLI::current_lines() const
{
    QReadLocker locker(&m_d->m_lock);
    std::vector<std::string> lrc_lines;

    // serialize the logical lyrics back to .lrc only to print it back
    // utilizes a const reference to strictly avoid copying
    for (const lyric &single_timestamp_line : m_d->m_lyrics) {
        lrc_lines.emplace_back(
            "[" + single_timestamp_line.ts.as_string() + "] " 
            + single_timestamp_line.text.toStdString());
    };

    return lrc_lines;
}

QFuture<void>
LyricsManifestLI::repopulate_with_lyrics_for_file(const QString &source)
{
    return m_d->read_from_metadata_tag(source).then(
        [](filelines lines) {
            std::vector<lyric> decoupled_lines;

            for (const std::string &line : lines) {
                std::vector<timestamp> prolly_single_timestamp = lines::line_timestamps(line);
                timestamp timestamp_to_use;

                if (prolly_single_timestamp.size() != 1) [[unlikely]] {
                    qCWarning(l_lyricsmanifest) << "The syrinc lyrics submodule did not unwrap the lyrics correctly. "
                        << "This should not happen, and lyrics progression may not work as expected.";

                    if (prolly_single_timestamp.empty()) {
                        timestamp_to_use = timestamp(0);
                    } else {
                        timestamp_to_use = prolly_single_timestamp.at(0);
                    }
                } else [[likely]] {
                    timestamp_to_use = prolly_single_timestamp.at(0);
                }

                decoupled_lines.emplace_back(
                    timestamp_to_use,
                    QString::fromStdString(tokens::trim_string(lines::strip_timestamps(line)))
                );
            }

            return decoupled_lines;
        }
    ).then(this, [this](std::vector<lyric> lines_to_add) {
        beginResetModel();

        {
            QWriteLocker locker(&m_d->m_lock);
            m_d->m_lyrics = std::move(lines_to_add);
        }

        endResetModel();
    });
}

void
LyricsManifestLI::clear()
{
    if (m_d->m_lyrics.empty())
        return;

    beginResetModel();
    {
        QWriteLocker locker(&m_d->m_lock);
        m_d->m_lyrics.clear();
    }
    endResetModel();
}