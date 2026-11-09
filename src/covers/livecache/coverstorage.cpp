#include "coverextract.hpp"
#include "coverstorage.hpp"
#include "covertransform.hpp"
#include "coveruris.hpp"
#include "hashing.hpp"
#include "mediatypes.hpp"
#include "pixelformats.hpp"
#include "thumbnails.hpp"

#include <QtConcurrent/QtConcurrent>

namespace covers::live {


cover_storage_LI::cover_storage_LI()
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
    m_response_pool.setExpiryTimeout(30000);
    m_response_pool.setThreadPriority(QThread::Priority::LowPriority);
}

cover_storage_LI::~cover_storage_LI ()
{
    m_response_pool.waitForDone();
}

cover_storage_LI::cache_slot
cover_storage_LI::slot_for (const QUrl &media_source, const QSize &size)
{
    // the same hash that names the thumbnail on disk is the memory cache key
    QString key = covers::hash::payload_and_size(toBase64Url(media_source), size);

    cache_shard &shard = *m_shards[qHash(key) % SHARD_COUNT];

    return { shard, std::move(key) };
}

QImage
cover_storage_LI::lookup_memory (const QUrl &media_source, const QSize &size)
{
    const cache_slot slot = slot_for(media_source, size);

    QImage img;
    {
        // copy under the lock: the cache may evict the entry the moment we let go
        QMutexLocker locker(&slot.shard.lock);
        if (const QImage *cached = slot.shard.cache.object(slot.key))
            img = *cached;
    }

    if (!img.isNull())
        qCDebug(l_coverprovider) << "Stored/overwritten by memory cache lookup.";

    return img;
}

QImage
cover_storage_LI::lookup_disk (const QUrl &media_source, const QSize &size)
{
    QImage img = covers::disk::fetch_thumbnail(media_source, size);

    // No thumbnail at this size: an original kept at full resolution ({0, 0}) can still be
    // cropped and rescaled by the caller. For size == {0, 0} it would be the same probe.
    if (img.isNull() && !size.isEmpty())
        img = covers::disk::fetch_thumbnail(media_source, QSize(0, 0));

    if (!img.isNull())
        qCDebug(l_coverprovider) << "Stored/overwritten by reading from the thumbnails cache.";

    return img;
}

QImage
cover_storage_LI::lookup_source (const QUrl &media_source)
{
    const QByteArray local_path = media_source.toLocalFile().toUtf8();
    TagLib::FileRef file(local_path.constData());

    if (file.isNull()) return QImage();

    QImage img = extract_cover(file.file());
    if (!img.isNull()) {
        qCDebug(l_coverprovider) << "Stored/overwritten on demand decoding.";
        return img;
    }

    qCDebug (l_coverprovider) << "Trying to make a thumbnail." << media_source;
    // try decoding a video frame, if possible
    img = make_thumbnail(media_source);
    if (!img.isNull())
        qCDebug(l_coverprovider) << "Made a thumbnail from a video file.";

    return img;
}

QQuickImageResponse *
cover_storage_LI::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto state = std::make_shared<cover_response_state>();

    // Built in this call, so it has the same thread affinity as the response.
    mailbox_ptr mailbox(new QObject);

    auto *response = new cover_image_response(state);

    m_response_pool.start(new cover_resolve_task(
        !disabled ? this : nullptr, id.toUtf8(), requestedSize,
        state, QPointer<cover_image_response>(response), std::move(mailbox)));

    return response;
}

QFuture<QImage>
cover_storage_LI::resolve(const QUrl &media_source, const QSize &requestedSize)
{
    return QtConcurrent::run(&m_response_pool, [this, media_source, requestedSize]() {
        return resolve_blocking(media_source, requestedSize);
    });
}

// store for whatever size cover_from_metadata is
bool
cover_storage_LI::store (
    const QUrl &media_source, // source payload
    const QVariant &cover_from_metadata,
    bool save_to_disk_cache
)
{
    if (disabled) return false;
    
    if (!cover_from_metadata.canConvert<QImage>()) {
        return false;
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return false;
    }

    const cache_slot slot = slot_for(media_source, img.size());

    {
        // lock only this specific shard
        QMutexLocker locker(&slot.shard.lock);
        const bool cached = slot.shard.cache.insert(slot.key, new QImage(img), img.sizeInBytes());
        if (!cached) {
            qCWarning(l_coverprovider) << "Cover for " << media_source
                                       << " exceeds shard max cost; not kept in memory.";
        }
    } // shard lock released immediately

    // cheapest test first: the file check costs a mkpath and a stat
    if (save_to_disk_cache && !disk::thumbnail_file_exists(media_source, img.size())) {
        covers::disk::write_thumbnail(media_source, img);
    }

    return true;
}

QImage
cover_storage_LI::resolve_blocking (
    const QUrl &media_source,
    const QSize &requestedSize
)
{
    if (media_source.isEmpty()) return QImage();

    // No usable size (invalid, or 0 on a side) means full resolution, spelled {0, 0} from here on
    const QSize target = requestedSize.isEmpty() ? QSize(0, 0) : requestedSize;

    // Lookups, cheapest first; the first one that finds something wins.
    // A memory hit is already finished: exactly the right size and already stored.
    QImage img = lookup_memory(media_source, target);
    const bool already_cached = !img.isNull();

    if (img.isNull()) img = lookup_disk(media_source, target);
    if (img.isNull()) img = lookup_source(media_source);

    if (img.isNull()) {
        qCDebug(l_coverprovider) << "No image resolved.";
    } else if (target.isEmpty()) {
        qCDebug(l_coverprovider) << "Full res resource returned by not specifying a requestedSize..";
    } else {
        // The transforms: crop largest aspect + resize = center crop and rescale.
        // The center bit comes because crop_largest_aspect fits the largest centered N:M rect.
        // Skipped when the image already is exactly the requested size (every cache hit).
        if (img.size() != target) {
            img = transform::crop_largest_aspect(img, target.width(), target.height());
            img = transform::lanczos_resize(img,
                                            static_cast<size_t>(target.width()),
                                            static_cast<size_t>(target.height()));
        }

        // The single store, after the transforms: the next request at this size is a memory hit.
        if (!already_cached) {
            store(media_source, img);
        }

        qCDebug(l_coverprovider) << "Returned an image of size " << img.width() << "x" << img.height();
    }

    return img;
}

bool
cover_storage_LI::is_cached(const QUrl &media_source, QSize size)
{
    const cache_slot slot = slot_for(media_source, size);

    QMutexLocker locker(&slot.shard.lock);
    return slot.shard.cache.contains(slot.key);
}

}
