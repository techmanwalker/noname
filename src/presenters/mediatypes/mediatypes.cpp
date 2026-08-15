#include "mediatypes.hpp"
#include "standardpaths.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

#include <cstddef>

Q_LOGGING_CATEGORY(l_mediatypes, "noname.mediatypes")


namespace Types {

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
    if (!m_hash.isEmpty()) return m_hash;

    if (m_source.isEmpty()) {
        // better to prevent
        qCFatal (l_mediatypes) << "Attempted to create a hash for a cover reference with an empty source.";
    }

    const QByteArray key = m_source.toLocalFile().toUtf8()
                          + ':' + QByteArray::number(qulonglong(m_square_size));

    m_hash = QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex());

    return m_hash;
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

QString
CoverRef::thumbnail_path  () const
{
    // temporary ref, no thumnbail
    if (hash().isEmpty()) return QString();

    QDir thumb_dir (dir(standardpaths::standard_dirs::thumbnails));
    thumb_dir.mkpath(".");
    return thumb_dir.absoluteFilePath(hash() + QStringLiteral(".jxl"));
}

bool
CoverRef::thumbnail_file_exists () const
{
    return QFile::exists(thumbnail_path());
}