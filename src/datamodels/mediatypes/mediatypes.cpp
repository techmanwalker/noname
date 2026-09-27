#include "mediatypes.hpp"
#include "coveruris.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

#include <cstddef>
#include <qobject.h>
#include <qurl.h>

Q_LOGGING_CATEGORY(l_mediatypes, "noname.mediatypes")


namespace Types {

quint64
Album::duration() const {
    quint64 total = 0;
    for (const Song &s : songs)
        total += s.duration;
    return total;
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
    quint64 total_seconds =  duration / 1000;

    quint64 mm = total_seconds / 60;
    quint8 ss = total_seconds % 60;

    return QString::number(mm) + ":" + (ss < 10 ? "0" : "") + QString::number(ss);
}

QString
Song::printable_joint_metadata () const
{
    return artist + (album.length() > 0 ? " · " + album : "");
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

CoverRef::CoverRef (const QUrl &source_media_path, size_t square_size)
    : m_source(source_media_path),
      m_square_size(square_size)
{
}

QString
CoverRef::encode_base64url() const
{
    if (m_source.isEmpty()) return QString();

    // Serialize as: "size|url"
    const QByteArray payload = QByteArray::number(m_square_size) + '|' + m_source.toEncoded();
    
    // OmitTrailingEquals prevents padding characters ('=') which clutter the URL
    return QString::fromLatin1(payload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

CoverRef 
CoverRef::decode_base64url(const QString &base64url_coverref)
{
    if (base64url_coverref.isEmpty()) return CoverRef(QUrl());

    const QByteArray payload = QByteArray::fromBase64(
        base64url_coverref.toLatin1(), 
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals
    );
    
    const qsizetype sep_idx = payload.indexOf('|');
    if (sep_idx == -1) return CoverRef(QUrl()); // Invalid format safeguard

    // Qt 6 idiomatic substring extraction
    const size_t size = payload.first(sep_idx).toULongLong();
    const QUrl url = QUrl::fromEncoded(payload.sliced(sep_idx + 1));

    return CoverRef(url, size);
}

QUrl
CoverRef::source () const
{
    return m_source;
}

size_t
CoverRef::size() const
{
    return m_square_size;
}

QUrl
CoverRef::uri () const
{
    const QString prolly_encoded = encode_base64url();
    return QUrl(covers::schema + (prolly_encoded.isEmpty() ? covers::default_cover_uri : prolly_encoded));
}