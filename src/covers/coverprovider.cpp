#include "coverprovider.hpp"
#include "coverstorage.hpp" // Included ONLY here

namespace covers::live {

cover_provider::cover_provider(std::shared_ptr<covers::live::cover_storage> realProvider)
    : QQuickAsyncImageProvider(),
      m_real(std::move(realProvider)) 
{
}

QQuickImageResponse*
cover_provider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return m_real->requestImageResponse(id, requestedSize);
}

void
cover_provider::register_source(const QString &hash, const QUrl &source, size_t crop_and_resize)
{
    m_real->register_source(hash, source, crop_and_resize);
}

bool
cover_provider::store(const QString &hash, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    return m_real->store(hash, cover_from_metadata, save_to_disk_cache);
}

} // namespace covers::live