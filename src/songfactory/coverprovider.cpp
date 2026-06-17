#include "coverprovider.hpp"

#include <QUuid>

cover_provider::cover_provider ()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QString
cover_provider::store(const QVariant &cover_from_metadata)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return QString();
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return QString();
    }

    // generate uuid for this cover
    QString uid = QUuid::createUuid().toString(QUuid::Id128);

    // acquire tomic lock (fast loop on cpu, no syscalls)
    while (m_spin_lock.test_and_set(std::memory_order_acquire)) {
        // very short active wait
    }
    
    // guarantee contiguous memory
    m_linear_cache.push_back({uid, std::move(img)});

    // free lock
    m_spin_lock.clear(std::memory_order_release);

    return uid;
}

QImage
cover_provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // 'id' contains the fragment that goes after "image://covers/"
    // as previous covers are immutable and are not reallocated, concurrently reading is safe
    for (const auto &pair : m_linear_cache) {
        if (pair.first == id) {
            QImage img = pair.second;
            if (size) {
                *size = img.size();
            }
            return img;
        }
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