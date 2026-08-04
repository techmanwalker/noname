#include "coverproviderproxy.hpp"

// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
// Please don't pay too much attention to the syntax here. Qt threatened me to do this.
__cover_provider_PROXY::__cover_provider_PROXY(
    std::shared_ptr<covers::live::cover_provider> realProvider
    )
    : QQuickAsyncImageProvider(),
      m_real(realProvider) 
{
}

// Simply redirect requests to the real cover provider.
QQuickImageResponse *
__cover_provider_PROXY::requestImageResponse(
    const QString &id,
    const QSize &requestedSize)
{
        return m_real->requestImageResponse(id, requestedSize);
}