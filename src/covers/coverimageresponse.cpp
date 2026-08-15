#include "coverstorage.hpp"

cover_image_response::cover_image_response(covers::live::cover_storage *provider, const QString &id, const QSize &requestedSize)
    : m_provider(provider), m_id(id), m_requested_size(requestedSize)
{
    // Required: QQuickImageResponse manages its own lifetime (deleteLater()
    // once finished() is emitted) — letting QThreadPool also auto-delete
    // us would be a double free.
    setAutoDelete(false);
}

QQuickTextureFactory *
cover_image_response::textureFactory() const
{
    // Null on a cancelled/unresolved response — QtQuick handles that
    // fine, it just renders nothing for this request.
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void 
cover_image_response::cancel()
{
    m_canceled.storeRelaxed(1);
}

void
cover_image_response::run()
{
    if (!m_canceled.loadRelaxed()) {
        m_image = m_provider->resolve_blocking(m_id, m_requested_size);
    }
    // Must still emit finished() even when cancelled, so the engine can clean us up.
    emit finished();
}