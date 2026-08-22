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

bool
cover_provider::store(const CoverRef &ref, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    return m_real->store(ref, cover_from_metadata, save_to_disk_cache);
}

} // namespace covers::live