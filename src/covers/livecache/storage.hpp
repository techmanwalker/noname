#pragma once

#include <QCache>
#include <QFuture>
#include <QLoggingCategory>
#include <QMutex>
#include <QPointer>
#include <QQuickAsyncImageProvider>
#include <QReadWriteLock>
#include <QRunnable>
#include <QThreadPool>
#include <memory>

#include "virtual/coverstorage.hpp"

// covers::live: hot cache on memory right away to consume, already decoded and yet but cheap to decode

namespace covers::live {

// cache structured like this to allow multiple responses at once to qml
struct cache_shard {
    QMutex lock;
    QCache<QString, QImage> cache;
};

// Enables loading embedded thumbnails to memory.
class cover_storage_LI : public QQuickAsyncImageProvider, public cover_storage
{
    
public:
    // Initialize indicating that this provider provides QImage objects
    cover_storage_LI();
    ~cover_storage_LI(); // shutdown and pool drain

    // Save picture and return a generated UUID
    bool store(const QUrl &media_source, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

    /*  Called by QtQuick to kick off an (async) image request; the actual
        resolution work runs on m_response_pool, off the GUI thread. */
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    bool is_cached (const QUrl &media_source, QSize size);

    /*  Blocking resolution chain: memory cache -> disk cache -> decode from
    registered source. Called from a worker thread only
    (cover_image_response::run()) — never the GUI thread. */
    QImage resolve_blocking(const QUrl &media_source, const QSize &requestedSize);

    bool disabled = false; // quickly respond with empty QImages effectively disabling responses

    QFuture<QImage> resolve (const QUrl &media_source, const QSize &requestedSize) override;

private:
    // Where a (media_source, size) pair lives in the memory cache. The only place that hashes the
    // pair and picks the shard, so a lookup can never look in a shard the store didn't write to.
    struct cache_slot {
        cache_shard &shard;
        QString      key;
    };
    cache_slot slot_for (const QUrl &media_source, const QSize &size);

    // The three places a cover can come from, cheapest first. A miss is a null QImage.
    QImage lookup_memory (const QUrl &media_source, const QSize &size);
    QImage lookup_disk   (const QUrl &media_source, const QSize &size);
    QImage lookup_source (const QUrl &media_source);

    static constexpr qsizetype SHARD_COUNT = 16;
    std::array<std::unique_ptr<cache_shard>, SHARD_COUNT> m_shards;

    QThreadPool m_response_pool; // dedicated so cover decoding never queues behind (or blocks) song_factory's metadata scan
};

}


// Shared by the response and its worker; whichever dies first, the other
// still has valid memory. image is written by the worker strictly before
// finished() is delivered and read only after it.
struct cover_response_state {
    QImage    image;
    QAtomicInt canceled { 0 };
};

// Lives on the QML pixmap reader thread. Never touched by worker threads.
// Deliberately no Q_OBJECT: no new signals/slots beyond QQuickImageResponse's.
class cover_image_response : public QQuickImageResponse
{
public:
    explicit cover_image_response(std::shared_ptr<cover_response_state> state);

    QQuickTextureFactory *textureFactory() const override;
    void cancel() override;

    // Reader thread only (called through the mailbox).
    void deliver() { emit finished(); }

private:
    std::shared_ptr<cover_response_state> m_state;
};

struct deferred_delete {
    void operator()(QObject *o) const { o->deleteLater(); }
};
using mailbox_ptr = std::unique_ptr<QObject, deferred_delete>;

// Pure worker: not a QObject, auto-deleted by the pool.
class cover_resolve_task : public QRunnable
{
public:
    cover_resolve_task(covers::live::cover_storage_LI *storage,
                       QByteArray id, QSize size,
                       std::shared_ptr<cover_response_state> state,
                       QPointer<cover_image_response> response,
                       mailbox_ptr mailbox);

    void run() override;

private:
    covers::live::cover_storage_LI       *m_storage;
    QByteArray                            m_id;
    QSize                                 m_size;
    std::shared_ptr<cover_response_state> m_state;
    QPointer<cover_image_response>        m_response;
    mailbox_ptr                           m_mailbox;
};
