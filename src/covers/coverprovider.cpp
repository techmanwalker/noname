#include "coverprovider.hpp"
#include "coverstorage.hpp" // Included ONLY here

namespace covers::live {

cover_providerLI::cover_providerLI(std::shared_ptr<covers::live::cover_storage> realProvider)
    : QQuickAsyncImageProvider(),
      m_real(std::move(realProvider)) 
{
}

QQuickImageResponse*
cover_providerLI::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    return m_real->requestImageResponse(id, requestedSize);
}

void
cover_providerLI::register_cover_reference(const CoverRef &ref)
{
    m_real->register_cover_reference(ref);
}

bool
cover_providerLI::store(const CoverRef &ref, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    return m_real->store(ref, cover_from_metadata, save_to_disk_cache);
}

} // namespace covers::live