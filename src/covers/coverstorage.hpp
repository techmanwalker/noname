#pragma once

#include <QCache>
#include <QLoggingCategory>
#include <QMutex>
#include <QQuickAsyncImageProvider>
#include <QReadWriteLock>
#include <QThreadPool>

Q_DECLARE_LOGGING_CATEGORY(l_coverprovider)

// covers::live: hot cache on memory right away to consume, already decoded and yet but cheap to decode

class CoverRef;

namespace covers::live {

// cache structured like this to allow multiple responses at once to qml
struct cache_shard {
    QMutex lock;
    QCache<QString, QImage> cache;
};

// Enables loading embedded thumbnails to memory.
class cover_storage : public QQuickAsyncImageProvider
{
    
public:
    // Initialize indicating that this provider provides QImage objects
    cover_storage();
    ~cover_storage(); // shutdown and pool drain

    // Save picture and return a generated UUID
    bool store(const CoverRef &ref, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

    /*  Called by QtQuick to kick off an (async) image request; the actual
        resolution work runs on m_response_pool, off the GUI thread. */
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    bool is_cached (const CoverRef &ref);

    /*  Blocking resolution chain: memory cache -> disk cache -> decode from
    registered source. Called from a worker thread only
    (cover_image_response::run()) — never the GUI thread. */
    QImage resolve_blocking(const QString &base64url_coverref, const QSize &requestedSize);

    bool disabled = false; // quickly respond with empty QImages effectively disabling responses

private:

    static constexpr size_t SHARD_COUNT = 16;
    std::array<std::unique_ptr<cache_shard>, SHARD_COUNT> m_shards;

    QThreadPool m_response_pool; // dedicated so cover decoding never queues behind (or blocks) song_factory's metadata scan
};

}


// Runs on cover_storage's m_response_pool. Deliberately no Q_OBJECT here:
// it adds no new signals/slots beyond the ones QQuickImageResponse already
// declares, so it needs no moc processing.
class cover_image_response : public QQuickImageResponse, public QRunnable
{
public:
    cover_image_response(covers::live::cover_storage *cover_provider, const QString &id, const QSize &requestedSize);

    QQuickTextureFactory *textureFactory() const override;

    void cancel() override;

    void run() override;

private:
    covers::live::cover_storage *m_provider;
    QString m_id;
    QSize m_requested_size;
    QImage m_image;
    QAtomicInt m_canceled { 0 };
};