#pragma once

#include <QCache>
#include <QLoggingCategory>
#include <QMutex>
#include <QPointer>
#include <QQuickAsyncImageProvider>
#include <QReadWriteLock>
#include <QRunnable>
#include <QThreadPool>
#include <memory>

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
    cover_resolve_task(covers::live::cover_storage *storage,
                       QString id, QSize size,
                       std::shared_ptr<cover_response_state> state,
                       QPointer<cover_image_response> response,
                       mailbox_ptr mailbox);

    void run() override;

private:
    covers::live::cover_storage          *m_storage;
    QString                               m_id;
    QSize                                 m_size;
    std::shared_ptr<cover_response_state> m_state;
    QPointer<cover_image_response>        m_response;
    mailbox_ptr                           m_mailbox;
};