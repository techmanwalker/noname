#include "coverproviderproxy.hpp"

// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
// Please don't pay too much attention to the syntax here. Qt threatened me to do this.
__cover_provider_PROXY::__cover_provider_PROXY(
    std::shared_ptr<cover_provider> realProvider
    )
    : QQuickImageProvider(QQuickImageProvider::Image),
      m_real(realProvider) 
{
}

// Simply redirect requests to the real cover provider.
QImage 
__cover_provider_PROXY::requestImage(
    const QString &hash,
    QSize *size,
    const QSize &requestedSize)
{
        return m_real->requestImage(hash, size, requestedSize);
}