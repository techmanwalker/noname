#include "coverextract.hpp"
#include "coverstorage.hpp"
#include "mediatypes.hpp"
#include "thumbnails.hpp"
#include <qimage.h>
#include <qmutex.h>

Q_LOGGING_CATEGORY(l_coverprovider, "noname.coverprovider")


namespace covers {
namespace live {


cover_storage::cover_storage ()
    : QQuickAsyncImageProvider()
{
    /*  Budget: 256 thumbnails' worth of a 256x256 RGBA8888 image (~64 MiB).
        or a little list when full res covers take up space for ~1 MiB each one. */
    static const qsizetype thumbnail_cost = QImage(256, 256, QImage::Format_RGBA8888).sizeInBytes();
    m_cache.setMaxCost(96 * thumbnail_cost);

    // all response calls here
    m_response_pool.setMaxThreadCount(std::max(2, QThread::idealThreadCount() / 2));
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

    {
        QMutexLocker locker(&m_cache_lock);
        const bool cached = m_cache.insert(ref.hash(), new QImage(img), cost);
        if (!cached) {
            qCWarning(l_coverprovider) << "Cover for" << ref.hash() << "exceeds the cache's max cost; not kept in memory.";
        }
    } // lock safely released

    if (!ref.thumbnail_file_exists() && save_to_disk_cache) {

        // async, won't block
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
    QImage img = QImage();

    {
        QMutexLocker locker (&m_cache_lock);

        QImage *cached_ptr = m_cache.object(ref.hash());
        // Copy while still locked: QImage is implicitly shared, so this copy is
        // just a refcount bump, but it's what keeps the pixel buffer alive if
        // another thread's insert() evicts (deletes) this entry right after we unlock.

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
cover_storage::is_cached (const CoverRef &ref)
{
    {
        QMutexLocker locker (&m_cache_lock);
        return m_cache.contains(ref.hash());
    }
}

}
}