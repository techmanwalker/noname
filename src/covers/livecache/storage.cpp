#include "decodeedit/extract.hpp"
#include "livecache/storage.hpp"
#include "decodeedit/transform.hpp"
#include "coveruris.hpp"
#include "livecache/hashing.hpp"
#include "mediatypes.hpp"
#include "decodeedit/pixelformats.hpp"
#include "thumbnaildiskio/thumbnails.hpp"
#include "logging/l_coverprovider.hpp"

#include <QtConcurrent/QtConcurrent>

#include <sys/resource.h>

namespace covers::live {

namespace {

// fewer thrashes during scrolling
qsizetype min_budget()
{
    static const qsizetype rowthumb_cost = QImage(
        default_cover_rowthumb_size,
        pixelformat_qimage
    ).sizeInBytes();

    static const qsizetype card_cost = QImage(
        default_cover_card_size,
        pixelformat_qimage
    ).sizeInBytes();

    static const qsizetype backdrop_cost = QImage(
        // do NOT set directly default_cover_background_size
        QSize(default_cover_background_size.height(),
              default_cover_background_size.height()),
        pixelformat_qimage
    ).sizeInBytes();

    return 2048 * rowthumb_cost + 256 * card_cost + 64 * backdrop_cost;
}
}

cover_storage_LI::cover_storage_LI()
    : QQuickAsyncImageProvider()
{
    for (auto& shard : m_shards)
        shard = std::make_unique<cache_shard>();

    set_memory_budget(min_budget());

    // leave cores for the GUI, render, decoder and RT threads
    m_response_pool.setMaxThreadCount(std::max(2, QThread::idealThreadCount() / 2));
    m_response_pool.setExpiryTimeout(30000);
}

cover_storage_LI::~cover_storage_LI ()
{
    m_response_pool.waitForDone();
}

void
cover_storage_LI::teardown ()
{
    // order matters: resolves feed store(), which feeds the thumbnail pool
    m_stop.request_stop();
    m_response_pool.waitForDone(); // queued tasks drain instantly: they see the stop and only deliver finished()
    covers::disk::teardown();      // queued writes dropped, in-flight encodes abort
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

    if (!img.isNull())
        qCDebug(l_coverprovider) << "Stored/overwritten by reading from the thumbnails cache.";

    return img;
}

QImage
cover_storage_LI::lookup_pending (const QUrl &media_source, const QSize &size)
{
    QImage img = covers::disk::pending_thumbnail(media_source, size);

    if (!img.isNull())
        qCDebug(l_coverprovider) << "Stored/overwritten from a write in progress.";

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

    if (m_stop.stop_requested()) return QImage();

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
        state, QPointer<cover_image_response>(response), std::move(mailbox)),
        next_priority());

    return response;
}

QFuture<QImage>
cover_storage_LI::resolve(const QUrl &media_source, const QSize &requestedSize)
{
    return QtConcurrent::task([this, media_source, requestedSize](QPromise<QImage> &promise) {
               promise.addResult(resolve_blocking(media_source, requestedSize,
                                                 [&promise] { return promise.isCanceled(); }));
           })
        .onThreadPool(m_response_pool)
        .withPriority(next_priority())
        .spawn();
}

// store for whatever size cover_from_metadata is
bool
cover_storage_LI::store (
    const QUrl &media_source, // source payload
    const QVariant &cover_from_metadata,
    bool save_to_disk_cache
)
{
    if (!cover_from_metadata.canConvert<QImage>()) return false;

    // stored under the image's own size
    const QImage img = cover_from_metadata.value<QImage>();
    return store_at(media_source, img.size(), img, save_to_disk_cache);
}

bool
cover_storage_LI::store_at (const QUrl &media_source, const QSize &key_size, const QImage &img,
                            bool save_to_disk_cache)
{
    if (disabled || m_stop.stop_requested() || img.isNull()) return false;

    const cache_slot slot = slot_for(media_source, key_size);

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
    if (save_to_disk_cache && !disk::thumbnail_file_exists(media_source, key_size)) {
        covers::disk::write_thumbnail(media_source, key_size, img, m_stop.get_token());
    }

    return true;
}

namespace {
// pool threads outlive tasks: demote each one once
void demote_this_thread()
{
    thread_local const bool done = [] { setpriority(PRIO_PROCESS, 0, 19); return true; }();
    (void) done;
}
}


QImage
cover_storage_LI::resolve_blocking (
    const QUrl &media_source,
    const QSize &requestedSize,
    const std::function<bool()> &stale)
{
    if (m_stop.stop_requested() || media_source.isEmpty()) return QImage();

    demote_this_thread();

    // 0 (or less) on a side leaves it to the cover's aspect ratio; {0, 0} is full resolution
    const QSize target { std::max(0, requestedSize.width()), std::max(0, requestedSize.height()) };

    // Lookups, cheapest first; the first one that finds something wins.
    // A memory hit is already finished: exactly the right size and already stored.
    QImage img = lookup_memory(media_source, target);
    const bool already_cached = !img.isNull();

    if (img.isNull() && m_stop.stop_requested()) return QImage();
    if (img.isNull()) img = lookup_pending(media_source, target);
    if (img.isNull()) img = lookup_disk(media_source, target);

    if (img.isNull() && (m_stop.stop_requested() || (stale && stale()))) return QImage();
    if (img.isNull()) img = lookup_source(media_source);

    if (img.isNull()) {
        qCDebug(l_coverprovider) << "No image resolved.";
    } else if (target.isNull()) {
        qCDebug(l_coverprovider) << "Full res resource returned by not specifying a requestedSize..";
    } else {
        // last exit before the heaviest CPU step
        if (m_stop.stop_requested()) return QImage();

        // The transforms: crop largest aspect + resize = center crop and rescale.
        // The center bit comes because crop_largest_aspect fits the largest centered N:M rect.
        // Skipped when the image already is exactly the requested size (every cache hit).
        const QSize final_size = transform::scaled_size(img.size(), target);
        if (img.size() != final_size) {
            // both sides given: crop to their aspect first; with a side open the cover's aspect stays
            if (!target.isEmpty())
                img = transform::crop_largest_aspect(img, target.width(), target.height());
            img = transform::lanczos_resize(img,
                                            static_cast<size_t>(final_size.width()),
                                            static_cast<size_t>(final_size.height()));
        }

        // keyed by the request, not by img.size(): they differ when a side is open
        if (!already_cached) {
            store_at(media_source, target, img);
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

qsizetype
cover_storage_LI::memory_budget_min ()
{
    return min_budget();
}

qsizetype
cover_storage_LI::set_memory_budget (qsizetype requested_bytes)
{
    QMutexLocker guard(&m_budget_lock);

    const qsizetype applied = std::max(requested_bytes, min_budget());
    const qsizetype per_shard = applied / SHARD_COUNT;

    for (auto &shard : m_shards) {
        QMutexLocker locker(&shard->lock);
        shard->cache.setMaxCost(per_shard); // trims LRU here if shrinking
    }

    m_budget.store(applied, std::memory_order_relaxed);
    return applied;
}

}
