#include "localdata.hpp"
#include "thumbnails.hpp"

#include <QDir>
#include <QFile>
#include <QThreadPool>

#include <QtConcurrent/QtConcurrent>

#include <algorithm>

namespace covers {

namespace disk {

Q_LOGGING_CATEGORY(l_localdata, "noname.memory.localdata")

namespace { // anonymous

// TGA header is always 18 bytes
#pragma pack(push, 1)
struct TgaHeader
{
    quint8  idLength        = 0;     // no image ID
    quint8  colorMapType    = 0;     // no color map
    quint8  imageType       = 2;     // uncompressed true-color
    quint16 colorMapStart   = 0;
    quint16 colorMapLength  = 0;
    quint8  colorMapDepth   = 0;
    quint16 xOrigin         = 0;
    quint16 yOrigin         = 0;
    quint16 width           = 0;
    quint16 height          = 0;
    quint8  pixelDepth      = 32;    // 8-8-8-8
    quint8  imageDescriptor = 0x28;  // top-left origin + 8-bit alpha
};
#pragma pack(pop)

static_assert(sizeof(TgaHeader) == 18, "TGA header must be 18 bytes");

// path(dirs::thumbnails) is the *directory*; this resolves the per-hash file
// inside it and guarantees the directory itself exists before use.
QString
thumbnail_file_path (const QString &hash)
{
    QDir thumb_dir (path(localdata::dirs::thumbnails).toLocalFile());
    thumb_dir.mkpath(".");
    return thumb_dir.absoluteFilePath(hash + QStringLiteral(".tga"));
}

// thumbnail encode+write thread pool; uastc is heavy
QThreadPool *
thumbnail_pool ()
{
    static QThreadPool pool;

    return &pool;
}

/*  The actual encode + disk write. Now purely an implementation detail —
    the public write_thumbnail() below only ever queues this onto
    thumbnail_pool(), it's never called directly. */
bool
write_thumbnail_blocking (const QString &hash, const QImage &thumbnail)
{
    if (thumbnail.isNull()) {
        qCWarning(l_localdata) << "refusing to write a null thumbnail for" << hash;
        return false;
    }

    const QImage rgba = thumbnail.format() == QImage::Format_RGBA8888
        ? thumbnail
        : thumbnail.convertToFormat(QImage::Format_RGBA8888);

    const int width  = rgba.width();
    const int height = rgba.height();
    const qsizetype tight_row_bytes = qsizetype(width) * 4;

    // Pack tightly (QImage may have padding)
    QByteArray packed;
    packed.resize(tight_row_bytes * height);
    for (int y = 0; y < height; ++y) {
        std::memcpy(packed.data() + y * tight_row_bytes,
                    rgba.constScanLine(y),
                    tight_row_bytes);
    }

    // Convert RGBA → BGRA (classic TGA order) in-place
    for (qsizetype i = 0; i < packed.size(); i += 4) {
        std::swap(packed[i + 0], packed[i + 2]); // R ↔ B
    }

    TgaHeader header;
    header.width  = static_cast<quint16>(width);
    header.height = static_cast<quint16>(height);

    const QString file_path = thumbnail_file_path(hash);
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(l_localdata) << "failed to open thumbnail for writing" << file_path
                                << file.errorString();
        return false;
    }

    if (file.write(reinterpret_cast<const char*>(&header), sizeof(header)) != sizeof(header) ||
        file.write(packed) != packed.size())
    {
        qCWarning(l_localdata) << "failed to write thumbnail data" << file_path
                                << file.errorString();
        file.remove();          // leave no partial file
        return false;
    }

    return true;
}

} // anonymous

/*  Deterministic, session-independent cache key for a song's thumbnail.
    Hashing the absolute path (not file contents) keeps this cheap — an MD5
    over a path-length string is microseconds, dwarfed by the decode/encode
    it lets us skip. crop_and_resize is folded in too, since the cache
    stores the already-resized image: without it, two call sites requesting
    different sizes for the same song would collide on one cache entry and
    silently serve the wrong resolution to whichever asked second. */
QString
thumbnail_hash_for (const QUrl &source, size_t crop_and_resize)
{
    const QByteArray key = source.toLocalFile().toUtf8()
                          + ':' + QByteArray::number(qulonglong(crop_and_resize));

    return QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex());
}


bool
has_thumbnail (const QString &hash)
{
    return QFile::exists(thumbnail_file_path(hash));
}

// Read the file ~/.local/share/noname/thumbnails/<hash>.tga and decode it
QImage
fetch_thumbnail (const QString &hash)
{
    const QString file_path = thumbnail_file_path(hash);

    if (!has_thumbnail(hash))
        return QImage();

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(l_localdata) << "failed to open thumbnail" << file_path
                                << file.errorString();
        return QImage();
    }

    TgaHeader header;
    if (file.read(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header)) {
        qCWarning(l_localdata) << "failed to read TGA header" << file_path;
        return QImage();
    }

    // Basic validation
    if (header.imageType != 2 || header.pixelDepth != 32 ||
        header.width == 0 || header.height == 0)
    {
        qCWarning(l_localdata) << "unsupported or corrupt TGA" << file_path
                                << "type=" << header.imageType
                                << "depth=" << header.pixelDepth;
        return QImage();
    }

    const qsizetype expected_size = qsizetype(header.width) * header.height * 4;
    QByteArray data = file.read(expected_size);
    if (data.size() != expected_size) {
        qCWarning(l_localdata) << "truncated size mismatch" << file_path
                                << "expected" << expected_size
                                << "got" << data.size();
        return QImage();
    }

    // BGRA → RGBA
    for (qsizetype i = 0; i < data.size(); i += 4) {
        std::swap(data[i + 0], data[i + 2]); // B ↔ R
    }

    // Create QImage and force a deep copy so the buffer stays valid
    QImage image(reinterpret_cast<const uchar*>(data.constData()),
                 header.width, header.height,
                 header.width * 4,
                 QImage::Format_RGBA8888);

    return image.copy();
}

QFuture<void>
write_thumbnail (const QString &hash, QImage thumbnail)
{
    return QtConcurrent::run(thumbnail_pool(), [hash, thumbnail] {
        write_thumbnail_blocking(hash, thumbnail);
    });
}

void
shutdown ()
{
    QThreadPool *pool = thumbnail_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already encoding finish naturally
}

}

}