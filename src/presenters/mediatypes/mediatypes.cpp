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
CoverRef::hash () const
{
    const QByteArray key = m_source.toLocalFile().toUtf8()
                          + ':' + QByteArray::number(qulonglong(m_square_size));

    return QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex());
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
    QString prolly_hashed = hash();
    return covers::schema + (prolly_hashed.isEmpty() ? covers::default_cover_uri : prolly_hashed);
}