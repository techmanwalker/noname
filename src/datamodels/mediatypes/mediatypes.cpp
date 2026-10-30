#include "mediatypes.hpp"
#include "coveruris.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

#include <cstddef>

Q_LOGGING_CATEGORY(l_mediatypes, "mediatypes")

namespace {

QString
printable_time (quint64 ms)
{
    quint64 total_seconds =  ms / 1000;

    quint64 mm = total_seconds / 60;
    quint8 ss = total_seconds % 60;

    return QString::number(mm) + ":" + (ss < 10 ? "0" : "") + QString::number(ss);
}

}


namespace Types {

quint64
Album::duration() const {
    quint64 total = 0;
    for (const Song &s : songs)
        total += s.duration;
    return total;
}

quint64
Playlist::duration() const {
    quint64 total = 0;
    for (const Song &s : songs)
        total += s.duration;
    return total;
}

QString
Playlist::duration_mmss() const {
    return printable_time(duration());
}

QUrl
Playlist::firstcover() const
{
    if (songs.size() == 0) return {};

    return songs.at(0).coveruri();
}

Directory::Directory (const QString &source_path)
    : path(source_path),
      title(QFileInfo(source_path).fileName())
{
}

// validity semantics

bool
Song::is_valid () const
{
    return !source.isEmpty();
}

QString
Song::duration_mmss () const
{
    return printable_time(duration);
}

QUrl
Song::coveruri () const 
{
    return cover.uri();
}

bool
Directory::is_valid () const
{
    return !path.isEmpty();
}

}

cover_payload::cover_payload (const QUrl &source_media_path, size_t square_size)
    : m_source(source_media_path),
      m_square_size(square_size)
{
}

QString
cover_payload::to_base64url() const
{
    if (m_source.isEmpty()) return QString();

    // Serialize as: "size|url"
    const QByteArray payload = QByteArray::number(m_square_size) + '|' + m_source.toEncoded();
    
    // OmitTrailingEquals prevents padding characters ('=') which clutter the URL
    return QString::fromLatin1(payload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}
 
cover_payload::cover_payload(const QString &base64url_payload)
{
    if (base64url_payload.isEmpty()) return;

    const QByteArray payload = QByteArray::fromBase64(
        base64url_payload.toLatin1(), 
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals
    );
    
    const qsizetype sep_idx = payload.indexOf('|');
    if (sep_idx == -1) return; // Invalid format safeguard

    // Qt 6 idiomatic substring extraction
    const size_t size = payload.first(sep_idx).toULongLong();
    const QUrl url = QUrl::fromEncoded(payload.sliced(sep_idx + 1));

    m_source = std::move(url);
    m_square_size = size;
}

QUrl
cover_payload::source () const
{
    return m_source;
}

size_t
cover_payload::size() const
{
    return m_square_size;
}

QUrl
cover_payload::uri () const
{
    const QString prolly_encoded = to_base64url();
    return QUrl(covers::schema + (prolly_encoded.isEmpty() ? covers::default_cover_uri : prolly_encoded));
}