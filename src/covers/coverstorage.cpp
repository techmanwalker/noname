#include "coverdecode.hpp"
#include "coverextract.hpp"
#include "coverstorage.hpp"
#include "coveruris.hpp"
#include "mediatypes.hpp"
#include "pixelformats.hpp"
#include "thumbnails.hpp"
#include <qhashfunctions.h>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")


namespace covers::live {


cover_storage::cover_storage()
    : QQuickAsyncImageProvider()
{
    // You MUST increase this total budget. 96 is not enough to prevent 
    // thrashing during scrolling. Let's allocate enough for ~1000 thumbnails.
    static const qsizetype thumbnail_cost = QImage(
        default_cover_thumbnail_size, 
        default_cover_thumbnail_size, 
        pixelformat_qimage
    ).sizeInBytes();

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
    auto *response = new cover_image_response(!disabled ? this : nullptr, id, requestedSize);
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
    CoverRef ref = CoverRef::decode_base64url(base64url_coverref);
    if (ref.source().isEmpty()) return QImage();

    QImage img;

    const size_t shard_idx = qHash(base64url_coverref) % SHARD_COUNT;
    auto& shard = m_shards[shard_idx];

    // Hot cache lookup
    {
        QMutexLocker locker(&shard->lock);
        QImage *cached_ptr = shard->cache.object(base64url_coverref);
        if (cached_ptr) {
            img = *cached_ptr;
            qCDebug(l_coverprovider) << "Stored/overwritten by memory cache lookup.";
        }
    }

    // Disk cache fallback
    if (img.isNull()) {
        img = covers::disk::fetch_thumbnail(ref);
        if (!img.isNull()) {
            store(ref, img);
            qCDebug(l_coverprovider) << "Stored/overwritten by reading from the thumbnails cache.";
        }
    }

    // On-demand decoding fallback
    if (img.isNull() && !ref.source().isEmpty()) {
        const QByteArray local_path = ref.source().toLocalFile().toUtf8();
        TagLib::FileRef file(local_path.constData());

        if (!file.isNull()) {
            img = extract_cover(file.file(), ref.size());
            if (!img.isNull()) {
                store(ref, img);
                qCDebug(l_coverprovider) << "Stored/overwritten on demand decoding.";
            }
        }
    }

    // Return empty if completely unresolved
    if (img.isNull()) {
        qCDebug(l_coverprovider) << "No image resolved.";
        return {};
    }

    // 4. Validate constraints and apply scaling once
    if (!requestedSize.isValid() || requestedSize.isEmpty()) {
        qCDebug(l_coverprovider) << "Full res resource returned by not specifying a requestedSize..";
        return img; // Unspecified size (-1, -1) or invalid = return full res
    }

    if (requestedSize.width() == requestedSize.height()) {
        int squareSize = requestedSize.width();
        qCDebug(l_coverprovider) << "Returned a square image of size " << squareSize;
        return decode::lanczos_resize_square(img, squareSize);
    }

    qCDebug(l_coverprovider) << "Returned a non square image of size " << requestedSize.width() << "x" << requestedSize.height();

    return decode::lanczos_resize (
        img,
        static_cast<size_t>(requestedSize.width()),
        static_cast<size_t>(requestedSize.height())
    );
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