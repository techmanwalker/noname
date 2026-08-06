#include "coverextract.hpp"
#include "coverprovider.hpp"
#include "thumbnails.hpp"

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")


namespace covers {
namespace live {


cover_provider::cover_provider ()
    : QQuickAsyncImageProvider()
{
    /*  Budget: 256 thumbnails' worth of a 256x256 RGBA8888 image (~64 MiB).
        or a little list when full res covers take up space for ~1 MiB each one. */
    static const qsizetype thumbnail_cost = QImage(256, 256, QImage::Format_RGBA8888).sizeInBytes();
    m_cache.setMaxCost(96 * thumbnail_cost);

    // all response calls here
    m_response_pool.setMaxThreadCount(std::max(2, QThread::idealThreadCount() / 2));
}

cover_provider::~cover_provider ()
{
    m_response_pool.waitForDone();
}

QQuickImageResponse *
cover_provider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto *response = new cover_image_response(this, id, requestedSize);
    m_response_pool.start(response);
    return response;
}

void
cover_provider::register_source(const QString &hash, const QUrl &source, size_t crop_and_resize)
{
    QWriteLocker locker(&m_sources_lock);
    m_sources.insert(hash, { source, crop_and_resize });
}

bool
cover_provider::store(const QString &hash, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return false;
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return false;
    }

    const qsizetype cost = img.sizeInBytes();

    // acquire atomic lock (fast loop on cpu, no syscalls)
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }

    // QCache takes ownership of the pointer. On failure (cost > maxCost,
    // i.e. a single cover bigger than the whole budget) it deletes it for
    // us — no manual cleanup needed either way.
    const bool cached = m_cache.insert(hash, new QImage(img), cost);

    // free lock
    m_spin_lock.clear(std::memory_order_release);

    if (!cached) {
        qCWarning(l_coverprovider) << "Cover for" << hash << "exceeds the cache's max cost; not kept in memory.";
    }

    if (!covers::disk::has_thumbnail(hash) && save_to_disk_cache) {

        // async, won't block
        covers::disk::write_thumbnail(hash, img);
    }

    return true;
}

QImage
cover_provider::resolve_blocking(const QString &id, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // Get from hot cache first

    // 'id' contains the fragment that goes after "image://covers/"
    // 'id' must correspond to a hash printed by song_factory::thumbnail_hash_for
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }

    QImage *cached_ptr = m_cache.object(id);
    // Copy while still locked: QImage is implicitly shared, so this copy is
    // just a refcount bump, but it's what keeps the pixel buffer alive if
    // another thread's insert() evicts (deletes) this entry right after we unlock.
    QImage img = cached_ptr ? *cached_ptr : QImage();

    m_spin_lock.clear(std::memory_order_release);

    if (!img.isNull()) {
        return img;
    }

    // If not in m_cache, fetch from disk
    img = covers::disk::fetch_thumbnail(id);

    if (!img.isNull()) {

        // qCDebug(l_coverprovider) << "Fetched thumbnail from disk cache for " << id;

        // Store in memory cache
        store (id, img);

        return img;
    }

    // Nothing cached anywhere — first time this cover has ever been
    // asked for. Look up where to decode it from.
    QUrl source;
    size_t crop_and_resize = 0;
    {
        QReadLocker locker(&m_sources_lock);
        auto it = m_sources.constFind(id);
        if (it != m_sources.constEnd()) {
            source = it->source;
            crop_and_resize = it->crop_and_resize;
        }
    }

    if (!source.isEmpty()) {
        const QByteArray local_path = source.toLocalFile().toUtf8();
        TagLib::FileRef file(local_path.constData());

        if (!file.isNull()) {
            img = extract_cover(file.file(), crop_and_resize);

            if (!img.isNull()) {
                // qCDebug(l_coverprovider) << "Decoded cover on demand for " << id;
                store(id, img);
                return img;
            }
        }
    }


    qCDebug (l_coverprovider) << "Could not fetch thumbnail from the requestImage standpoint.";

    // To be able to actually retrieve the default cover
    using namespace Qt::StringLiterals;

    // _s suffix creates a QString on compilation time cleanly without macros
    QImage default_cover(default_cover_uri);

    return default_cover;
}

bool
cover_provider::is_cached (const QString &hash)
{
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }

    const bool present = m_cache.contains(hash);

    m_spin_lock.clear(std::memory_order_release);

    return present;
}

}
}