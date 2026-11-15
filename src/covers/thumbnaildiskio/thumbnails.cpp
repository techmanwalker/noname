#include "decodeedit/codecs.hpp"
#include "decodeedit/pixelformats.hpp"
#include "livecache/hashing.hpp"
#include "mediatypes.hpp"
#include "standardpaths.hpp"
#include "thumbnaildiskio/thumbnails.hpp"

#include <QDir>
#include <QFile>
#include <QThreadPool>

#include <QtConcurrent/QtConcurrent>

namespace covers::disk {

Q_LOGGING_CATEGORY(l_thumbnails, "thumbnails")

namespace { // anonymous

// path(dirs::thumbnails) is the *directory*; this resolves the per-hash file
// inside it and guarantees the directory itself exists before use.

// thumbnail encode+write thread pool; uastc is heavy
QThreadPool *
thumbnail_pool ()
{
    static QThreadPool pool;

    return &pool;
}

} // anonymous

QString
thumbnail_file_path (
    const QUrl &media_source,
    QSize target_size
)
{
    QString fs_file_hash = covers::hash::payload_and_size(toBase64Url(media_source), target_size);
    if (fs_file_hash.isEmpty()) return QString();

    QDir thumb_dir (dir(standardpaths::standard_dirs::thumbnails));
    thumb_dir.mkpath(".");
    return thumb_dir.absoluteFilePath(fs_file_hash + QStringLiteral(".jxl"));
}

bool
thumbnail_file_exists(
    const QUrl &media_source,
    QSize target_size
)
{
    return QFile::exists(thumbnail_file_path(media_source, target_size));
}

// Read the file ~/.local/share/<project name>/thumbnails/<hash>.tga and decode it
QImage
fetch_thumbnail (
    const QUrl &media_source,
    QSize target_size
)
{
    return codecs::read_jpegxl(
        thumbnail_file_path(media_source.toLocalFile(), target_size)
    );
}

// Write the thumbnail for whatever size thumbnail is
QFuture<void>
write_thumbnail (
    const QUrl &media_source,
    QImage thumbnail
)
{
    return QtConcurrent::run(
        thumbnail_pool(),
        &codecs::write_jpegxl,
        thumbnail_file_path(media_source.toLocalFile(), thumbnail.size()),
        std::move(thumbnail),
        pixelformat_qimage
    );
}

void
teardown ()
{
    QThreadPool *pool = thumbnail_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already encoding finish naturally
}

}
