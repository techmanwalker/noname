#include "lyricsmanifest.hpp"
#include "lyrictypes.hpp"

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
#include <QtConcurrent/QtConcurrent>

#include <optional>

Q_LOGGING_CATEGORY(l_lyricsmanifest, "noname.lyrics");

using namespace syrinc;
using namespace syrinc::audio;
using namespace syrinc::timestamps;

// decoupled
struct __syrinc_lyric {
    syrinc::timestamps::timestamp ts;
    QString text;
};

// Projects the private, syrinc-coupled __syrinc_lyric into the public Lyric
// gadget; never hand the private type itself to QML.
static QVariant
to_lyric_gadget(const __syrinc_lyric &x)
{
    return QVariant::fromValue(Lyric{
        static_cast<quint64>(x.ts.as_ms()),
        x.text
    });
}

static const RoleDefinitions<__syrinc_lyric> lyrics_roles = {
    // Whole-gadget role: lets LyricDelegate.qml declare
    // `required property lyric model`, mirroring container_roles's
    // "model"/"modelData" for Types::Any.
    { "model", to_lyric_gadget },
    { "modelData", to_lyric_gadget }
};

namespace {

struct LyricLookup {
    std::optional<int> active_row; // index of the last line with ts <= ts_ms
    std::optional<int> next_row;   // index of the first line with ts >  ts_ms
};

// No locking here — callers hold whatever lock they need for as long as they
// use the result. Row indices, not pointers: nothing to keep alive, nothing
// to own, and it's the exact shape QModelIndex needs downstream.
LyricLookup
lookup_lyrics_at(const std::vector<__syrinc_lyric> &lyrics, quint64 ts_ms)
{
    auto it = std::upper_bound(
        lyrics.begin(), lyrics.end(), timestamp(ts_ms),
        [](const timestamp &pos, const __syrinc_lyric &line) {
            return pos.as_ms() < line.ts.as_ms();
        });

    LyricLookup result;
    if (it != lyrics.begin()) {
        // Walk left over any lines sharing the same timestamp so ties
        // resolve to the first line in file order.
        auto active_it = std::prev(it);
        while (active_it != lyrics.begin()
               && std::prev(active_it)->ts.as_ms() == active_it->ts.as_ms())
            --active_it;
        result.active_row = static_cast<int>(std::distance(lyrics.begin(), active_it));
    }
    if (it != lyrics.end())
        result.next_row = static_cast<int>(std::distance(lyrics.begin(), it));
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

    // Purely a diff-check for highlightedRowChanged — never consulted as
    // ground truth. index_of_first_highlighted_row() always recomputes
    // fresh, so a stale value here can cause at worst one skipped/extra
    // notify, self-correcting on the next poll. Nothing reads it as fact.
    int m_lastHighlightedRow = -1;

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

int
LyricsManifestLI::itemCount () const
{
    return rowCount();
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

        emit countChanged();

        if (m_d->m_lastHighlightedRow != -1) {
            m_d->m_lastHighlightedRow = -1;
            emit highlightedRowChanged();
        }
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

    if (m_d->m_lastHighlightedRow != -1) {
        m_d->m_lastHighlightedRow = -1;
        emit highlightedRowChanged();
    }
}

// Lookup

std::optional<quint64>
LyricsManifestLI::ts_of_lyric_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto row = lookup_lyrics_at(m_d->m_lyrics, ts_ms).active_row;
    if (!row) return std::nullopt;

    return static_cast<quint64>(m_d->m_lyrics[*row].ts.as_ms());
}

std::optional<quint64>
LyricsManifestLI::next_lyric_ts_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto row = lookup_lyrics_at(m_d->m_lyrics, ts_ms).next_row;
    if (!row) return std::nullopt;

    return static_cast<quint64>(m_d->m_lyrics[*row].ts.as_ms());
}

std::optional<QString>
LyricsManifestLI::lyric_at(quint64 ts_ms) const
{
    QReadLocker locker(&m_d->m_lock);
    const auto row = lookup_lyrics_at(m_d->m_lyrics, ts_ms).active_row;
    if (!row) return std::nullopt;

    return m_d->m_lyrics[*row].text;
}

QModelIndex
LyricsManifestLI::index_of_first_highlighted_row() const
{
    if (!ae)
        return {};

    const quint64 curr_ms = ae->current_position_ms();

    QReadLocker locker(&m_d->m_lock);
    const auto row = lookup_lyrics_at(m_d->m_lyrics, curr_ms).active_row;

    return row ? index(*row) : QModelIndex();
}

void
LyricsManifestLI::poll_highlighted_line_change()
{
    const int newRow = index_of_first_highlighted_row().row(); // -1 if none

    if (newRow == m_d->m_lastHighlightedRow)
        return;

    m_d->m_lastHighlightedRow = newRow;
    emit highlightedRowChanged();
}