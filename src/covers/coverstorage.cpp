#include "coverextract.hpp"
#include "coverstorage.hpp"
#include "mediatypes.hpp"
#include "pixelformats.hpp"
#include "thumbnails.hpp"

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")


namespace covers {
namespace live {


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

void
cover_storage::register_cover_reference(const CoverRef &ref)
{
    QWriteLocker locker(&m_sources_lock);
    m_refs.append(ref);
}

bool
cover_storage::store(const CoverRef &ref, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return false;
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return false;
    }

    const qsizetype cost = img.sizeInBytes();

    // calculate deterministic shard destination
    const size_t shard_idx = qHash(ref.hash()) % SHARD_COUNT;
    auto &shard = m_shards[shard_idx];

    {
        // lock only this specific shard
        QMutexLocker locker(&shard->lock);
        const bool cached = shard->cache.insert(ref.hash(), new QImage(img), cost);
        if (!cached) {
            qCWarning(l_coverprovider) << "Cover for" << ref.hash() 
                                       << "exceeds shard max cost; not kept in memory.";
        }
    } // shard lock released immediately

    if (!ref.thumbnail_file_exists() && save_to_disk_cache) {
        covers::disk::write_thumbnail(ref, img);
    }

    return true;
}

QImage
cover_storage::resolve_blocking(const QString &id, const QSize &requestedSize)
{
    CoverRef target_ref(QUrl(), 0);
    
    bool found = false;

    {
        // Protect the read iteration
        QReadLocker locker(&m_sources_lock);
        
        // Lean on C++20 ranges
        auto it = std::ranges::find_if(m_refs, [&id](const CoverRef& ref) {
            return ref.hash() == id;
        });

        if (it != m_refs.end()) {
            target_ref = *it;
            found = true;
        }
    } // Read lock released safely before heavy decoding

    if (found) {
        return resolve_blocking(target_ref, requestedSize);
    }

    return QImage();
}

QImage
cover_storage::resolve_blocking(const CoverRef &ref, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // Get from hot cache first
    // route to the correct shard based on the ID hash
    const size_t shard_idx = qHash(ref.hash()) % SHARD_COUNT;
    auto& shard = m_shards[shard_idx];

    QImage img;

    // lock ONLY this specific shard
    {
        QMutexLocker locker(&shard->lock);
        QImage *cached_ptr = shard->cache.object(ref.hash());
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
    // Route directly to the same shard
    const size_t shard_idx = qHash(ref.hash()) % SHARD_COUNT;
    auto &shard = m_shards[shard_idx];

    QMutexLocker locker(&shard->lock);
    return shard->cache.contains(ref.hash());
}

}
}