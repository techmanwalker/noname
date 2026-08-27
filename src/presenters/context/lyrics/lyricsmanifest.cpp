#include "lyricsmanifest.hpp"

// syrinc includes now strictly bound to the implementation file
#include "globals.hpp" 
#include "rolecompiler.hpp"
#include "timestamps.hpp"
#include "lines.hpp"
#include "mediatypes.hpp"
#include "metadata.hpp"
#include "process.hpp"
#include "tokens.hpp"

#include <QFile>
#include <QQmlEngine>
#include <QtConcurrent/QtConcurrent>
#include <optional>
#include <qfuture.h>
#include <qloggingcategory.h>
#include <qreadwritelock.h>

Q_LOGGING_CATEGORY(l_lyricsmanifest, "noname.lyrics");

using namespace syrinc;
using namespace syrinc::audio;
using namespace syrinc::timestamps;

// decoupled
struct __syrinc_lyric {
    syrinc::timestamps::timestamp ts;
    QString text;
    bool highlighted = false;
};

static const RoleDefinitions<__syrinc_lyric> lyrics_roles = {
    { "timestamp", [](const __syrinc_lyric &x) -> QVariant {
        return static_cast<qulonglong>(x.ts.as_ms());
    }},
    { "text", [](const __syrinc_lyric &x) -> QVariant {
        return x.text;
    }},
    { "highlighted", [](const __syrinc_lyric &x) -> QVariant {
        return x.highlighted;
    }}

};

namespace {

struct LyricLookup {
    const __syrinc_lyric *active = nullptr; // last line with ts <= ts_ms, or nullptr
    const __syrinc_lyric *next   = nullptr; // first line with ts >  ts_ms, or nullptr
};

// Caller must hold at least a read lock over `lyrics` for the duration of the call
// and for as long as it dereferences the returned pointers.
LyricLookup
lookup_lyrics_at(const std::vector<__syrinc_lyric> &lyrics, quint64 ts_ms)
{
    auto it = std::upper_bound(
        lyrics.begin(), lyrics.end(), timestamp(ts_ms),
        [](const timestamp &pos, const __syrinc_lyric &line) {
            return pos.as_ms() < line.ts.as_ms();
        });

    LyricLookup result;
    if (it != lyrics.begin())
        result.active = &*std::prev(it);
    if (it != lyrics.end())
        result.next = &*it;
    return result;
}

}

// Private implementation class sealing the internal components
class LyricsManifestPrivate {
public:
    LyricsManifestPrivate() : m_roles(lyrics_roles) {}

    std::vector<__syrinc_lyric> m_lyrics;
    CompiledRoleSet<__syrinc_lyric> m_roles;
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
LyricsManifestLI::LyricsManifestLI(QObject *parent, std::shared_ptr<audio_engine> position_tracker)
    : QAbstractListModel(parent),
      m_d(std::make_unique<LyricsManifestPrivate>()),
      ae(position_tracker)
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
    for (const __syrinc_lyric &single_timestamp_line : m_d->m_lyrics) {
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
            std::vector<__syrinc_lyric> decoupled_lines;

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
    ).then(this, [this](std::vector<__syrinc_lyric> lines_to_add) {
        beginResetModel();

        {
            QWriteLocker locker(&m_d->m_lock);
            m_d->m_lyrics = std::move(lines_to_add);
        }

        endResetModel();
    });
}

QFuture<void>
LyricsManifestLI::load_current_track_lyrics ()
{
    return repopulate_with_lyrics_for_file(ae->current_track().source.toLocalFile());
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

// Lookup

std::optional<quint64>
LyricsManifestLI::ts_of_lyric_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto *line = lookup_lyrics_at(m_d->m_lyrics, ts_ms).active;
    if (!line) return std::nullopt;

    return static_cast<quint64>(line->ts.as_ms());
}

std::optional<quint64>
LyricsManifestLI::next_lyric_ts_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto *line = lookup_lyrics_at(m_d->m_lyrics, ts_ms).next;
    if (!line) return std::nullopt;

    return static_cast<quint64>(line->ts.as_ms());
}

std::optional<QString>
LyricsManifestLI::lyric_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto *line = lookup_lyrics_at(m_d->m_lyrics, ts_ms).active;
    if (!line) return std::nullopt;

    return line->text;
}

void
LyricsManifestLI::poll_highlighted_line_change()
{
    if (!ae || m_d->m_lyrics.empty())
        return;

    auto &lyrics = m_d->m_lyrics;
    const quint64 now = ae->current_position_ms();

    const __syrinc_lyric *active;
    {
        QReadLocker locker(&m_d->m_lock);
        active = lookup_lyrics_at(lyrics, now).active;
    }

    const std::optional<int> highlighted_role = m_d->m_roles.roleNumber("highlighted");

    for (int row = 0; row < static_cast<int>(lyrics.size()); ++row) {
        const bool should_be_highlighted = (&lyrics[row] == active);

        if (lyrics[row].highlighted == should_be_highlighted)
            continue;

        {
            QWriteLocker locker(&m_d->m_lock);
            lyrics[row].highlighted = should_be_highlighted;
        }

        const QModelIndex idx = index(row);
        if (highlighted_role)
            emit dataChanged(idx, idx, {*highlighted_role});
        else
            emit dataChanged(idx, idx);
    }
}