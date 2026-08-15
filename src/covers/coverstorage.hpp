#pragma once

#include <QCache>
#include <QLoggingCategory>
#include <QQuickAsyncImageProvider>
#include <QReadWriteLock>
#include <QThreadPool>

Q_DECLARE_LOGGING_CATEGORY(l_coverprovider)

// covers::live: hot cache on memory right away to consume, already decoded and yet but cheap to decode

namespace covers {
namespace live {

// Enables loading embedded thumbnails to memory.
class cover_storage : public QQuickAsyncImageProvider
{
public:
    // Initialize indicating that this provider provides QImage objects
    cover_storage();
    ~cover_storage(); // shutdown and pool drain

    // Save picture and return a generated UUID
    bool store(const QString &hash, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

    /*  Called by QtQuick to kick off an (async) image request; the actual
        resolution work runs on m_response_pool, off the GUI thread. */
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    // save {hash -> the original source path and size it was calculated from}
    void register_source(const QString &hash, const QUrl &source, size_t crop_and_resize);

    bool is_cached (const QString &hash);

    // Address where cached covers are (should be) located
    static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

    // Default cover image uri
    static constexpr char default_cover_uri[] = "";

        /*  Blocking resolution chain: memory cache -> disk cache -> decode from
        registered source. Called from a worker thread only
        (cover_image_response::run()) — never the GUI thread. */
    QImage resolve_blocking(const QString &id, const QSize &requestedSize);

private:

    struct cover_source {
        QUrl source;
        size_t crop_and_resize;
    };

    QCache<QString, QImage> m_cache;

    /*  guards ALL cache access, not just insertion — QCache can evict (delete)
        an entry from any thread during insert(), so unlocked reads are no longer
        safe like they were with QHash */
    std::atomic_flag m_spin_lock = ATOMIC_FLAG_INIT; 

    QHash<QString, cover_source> m_sources;
    QReadWriteLock m_sources_lock; // separate lock from m_spin_lock: writes come in bursts during a scan, reads can come from many concurrent decode jobs at once

    QThreadPool m_response_pool; // dedicated so cover decoding never queues behind (or blocks) song_factory's metadata scan
};

}
}


// Runs on cover_storage's m_response_pool. Deliberately no Q_OBJECT here:
// it adds no new signals/slots beyond the ones QQuickImageResponse already
// declares, so it needs no moc processing.
class cover_image_response : public QQuickImageResponse, public QRunnable
{
public:
    cover_image_response(covers::live::cover_storage *provider, const QString &id, const QSize &requestedSize);

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