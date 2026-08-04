#include "coverprovider.hpp"
#include "thumbnails.hpp"

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")

cover_provider::cover_provider ()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    /*  Budget: 256 thumbnails' worth of a 256x256 RGBA8888 image (~64 MiB).
        or a little list when full res covers take up space for ~1 MiB each one. */
    static const qsizetype thumbnail_cost = QImage(256, 256, QImage::Format_RGBA8888).sizeInBytes();
    m_cache.setMaxCost(256 * thumbnail_cost);
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

    if (!localdata::has_thumbnail(hash) && save_to_disk_cache) {

        // async, won't block
        localdata::write_thumbnail(hash, img);
    }

    return true;
}

QImage
cover_provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
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
        if (size) {
            *size = img.size();
        }

        return img;
    }

    // If not in m_cache, fetch from disk
    img = localdata::fetch_thumbnail(id);

    if (!img.isNull()) {

        qCDebug(l_coverprovider) << "Fetched thumbnail from disk cache for " << id;

        // Store in memory cache
        store (id, img);

        if (size) {
            *size = img.size();
        }

        return img;
    }

    qCDebug (l_coverprovider) << "Could not fetch thumbnail from the requestImage standpoint.";

    // To be able to actually retrieve the default cover
    using namespace Qt::StringLiterals;

    // _s suffix creates a QString on compilation time cleanly without macros
    QImage default_cover(default_cover_uri);

    if (size) {
        *size = default_cover.size();
    }

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