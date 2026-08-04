#include "coverprovider.hpp"
#include "thumbnails.hpp"
#include <qloggingcategory.h>
#include <qobject.h>

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")

cover_provider::cover_provider ()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
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

    // acquire atomic lock (fast loop on cpu, no syscalls)
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }
    
    m_cache.insert(hash, img);

    if (!localdata::has_thumbnail(hash) && save_to_disk_cache) {

        // async, won't block
        localdata::write_thumbnail(hash, img);
    }

    // free lock
    m_spin_lock.clear(std::memory_order_release);

    return true;
}

QImage
cover_provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // Get from hot cache first

    // 'id' contains the fragment that goes after "image://covers/"
    // as previous covers are immutable and are not reallocated, concurrently reading is safe
    // 'id' must correspond to a hash printed by song_factory::thumbnail_hash_for
    auto it = m_cache.find(id);

    if (it != m_cache.end()) {
        QImage img = it.value();

        if (size) {
            *size = img.size();
        }

        return img;
    }

    // If not in m_cache, fetch from disk
    QImage img = localdata::fetch_thumbnail(id);

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
    return m_cache.contains(hash);
}