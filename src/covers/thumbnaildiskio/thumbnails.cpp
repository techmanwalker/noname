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
    static const bool configured = (pool.setMaxThreadCount(2), true);   // ~55 MB per encode
    (void) configured;

    return &pool;
}

} // anonymous

QString
thumbnail_file_path (
    const QUrl &media_source,
    QSize target_size
)
{
    // a path-only QUrl hashes differently from file:///… and would silently miss every cache
    Q_ASSERT_X(!media_source.scheme().isEmpty(), "covers::disk::thumbnail_file_path",
               "URL must contain a scheme");

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
    return codecs::read_jpegxl(thumbnail_file_path(media_source, target_size));
}

// stop aborts the encode, even mid-way; once an encoded buffer exists the write always completes
QFuture<void>
write_thumbnail (
    const QUrl &media_source,
    QImage thumbnail,
    std::stop_token stop
)
{
    return QtConcurrent::run(
        thumbnail_pool(),
        &codecs::write_jpegxl,
        thumbnail_file_path(media_source, thumbnail.size()),
        std::move(thumbnail),
        pixelformat_qimage,
        std::move(stop)
    );
}

void
teardown ()
{
    QThreadPool *pool = thumbnail_pool();
    pool->clear();          // queued, never started: dropped
    pool->waitForDone();    // in-flight encodes already saw the stop; a write in progress finishes
}

}
