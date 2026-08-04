#include "coverprovider.hpp"

cover_provider::cover_provider ()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

bool
cover_provider::store(const QVariant &cover_from_metadata, const QString &id)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return false;
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return false;
    }

    // acquire atomic lock (fast loop on cpu, no syscalls)
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }
    
    // guarantee contiguous memory
    m_cache.insert(id, std::move(img));

    // free lock
    m_spin_lock.clear(std::memory_order_release);

    return true;
}

QImage
cover_provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // 'id' contains the fragment that goes after "image://covers/"
    // as previous covers are immutable and are not reallocated, concurrently reading is safe
    auto it = m_cache.find(id);

    if (it != m_cache.end()) {
        QImage img = it.value();

        if (size) {
            *size = img.size();
        }

        return img;
    }


    // To be able to actually retrieve the default cover
    using namespace Qt::StringLiterals;

    // _s suffix creates a QString on compilation time cleanly without macros
    QImage default_cover(default_cover_uri);

    if (size) {
        *size = default_cover.size();
    }

    return default_cover;
}