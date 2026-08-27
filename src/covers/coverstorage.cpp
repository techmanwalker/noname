#include "coverextract.hpp"
#include "coverstorage.hpp"
#include "mediatypes.hpp"
#include "pixelformats.hpp"
#include "thumbnails.hpp"
#include <qhashfunctions.h>

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")


namespace covers::live {


cover_storage::cover_storage()
    : QQuickAsyncImageProvider()
{
    // You MUST increase this total budget. 96 is not enough to prevent 
    // thrashing during scrolling. Let's allocate enough for ~1000 thumbnails.
    static const qsizetype thumbnail_cost = QImage(256, 256, pixelformat_qimage).sizeInBytes();
    const qsizetype total_budget = 1024 * thumbnail_cost;
    const qsizetype budget_per_shard = total_budget / SHARD_COUNT;

    for (auto& shard : m_shards) {
        shard = std::make_unique<cache_shard>();
        shard->cache.setMaxCost(budget_per_shard);
    }

    m_response_pool.setMaxThreadCount(QThread::idealThreadCount());
}

cover_storage::~cover_storage ()
{
    m_response_pool.waitForDone();
}

QQuickImageResponse *
cover_storage::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto *response = new cover_image_response(this, id, requestedSize);
    m_response_pool.start(response);
    return response;
}

bool
cover_storage::store (
    const CoverRef &ref,
    const QVariant &cover_from_metadata,
    bool save_to_disk_cache
)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return false;
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return false;
    }

    const QString &base64url_coverref = ref.encode_base64url();

    const qsizetype cost = img.sizeInBytes();

    // calculate deterministic shard destination
    const size_t shard_idx = qHash(base64url_coverref) % SHARD_COUNT;
    auto &shard = m_shards[shard_idx];

    {
        // lock only this specific shard
        QMutexLocker locker(&shard->lock);
        const bool cached = shard->cache.insert(base64url_coverref, new QImage(img), cost);
        if (!cached) {
            qCWarning(l_coverprovider) << "Cover for " << ref.source()
                                       << " exceeds shard max cost; not kept in memory.";
        }
    } // shard lock released immediately

    if (!disk::thumbnail_file_exists(ref) && save_to_disk_cache) {
        covers::disk::write_thumbnail(ref, img);
    }

    return true;
}

QImage
cover_storage::resolve_blocking (
    const QString &base64url_coverref,
    const QSize &requestedSize
)
{
    Q_UNUSED(requestedSize);

    CoverRef ref = CoverRef::decode_base64url(base64url_coverref);

    if (ref.source().isEmpty()) return QImage();

    // Get from hot cache first
    // route to the correct shard based on the ID hash
    const size_t shard_idx = qHash(base64url_coverref) % SHARD_COUNT;
    auto& shard = m_shards[shard_idx];

    QImage img;

    // lock ONLY this specific shard
    {
        QMutexLocker locker(&shard->lock);
        QImage *cached_ptr = shard->cache.object(base64url_coverref);
        if (cached_ptr) {
            img = *cached_ptr;
        }
    }

    if (!img.isNull()) {
        return img;
    }

    // If not in m_cache, fetch from disk
    img = covers::disk::fetch_thumbnail(ref);

    if (!img.isNull()) {

        // qCDebug(l_coverprovider) << "Fetched thumbnail from disk cache for " << id;

        // Store in memory cache
        store (ref, img);

        return img;
    }

    // The reference tells us where to look for the coer


    if (!ref.source().isEmpty()) {
        const QByteArray local_path = ref.source().toLocalFile().toUtf8();
        TagLib::FileRef file(local_path.constData());

        if (!file.isNull()) {
            img = extract_cover(file.file(), ref.size());

            if (!img.isNull()) {
                // qCDebug(l_coverprovider) << "Decoded cover on demand for " << id;
                store(ref, img);
                return img;
            }
        }
    }

    // until we set a real null cover to display
    return QImage ();

    /*
    qCDebug (l_coverprovider) << "Could not fetch thumbnail from the requestImage standpoint.";

    // To be able to actually retrieve the default cover
    using namespace Qt::StringLiterals;

    // _s suffix creates a QString on compilation time cleanly without macros
    QImage default_cover(default_cover_uri);

    return default_cover;
    */
}

bool
cover_storage::is_cached(const CoverRef &ref)
{
    const QString base64url_coverref = ref.encode_base64url();
    // Route directly to the same shard
    const size_t shard_idx = qHash(base64url_coverref) % SHARD_COUNT;
    auto &shard = m_shards[shard_idx];

    QMutexLocker locker(&shard->lock);
    return shard->cache.contains(base64url_coverref);
}

}