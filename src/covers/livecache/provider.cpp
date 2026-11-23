#include "livecache/provider.hpp"
#include "livecache/storage.hpp"

namespace covers::live {

cover_provider::cover_provider(std::shared_ptr<covers::live::cover_storage_LI> realProvider)
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
cover_provider::store(const QUrl &media_source, const QVariant &cover_from_metadata, bool save_to_disk_cache)
{
    return m_real->store(media_source, cover_from_metadata, save_to_disk_cache);
}

qsizetype cover_provider::set_memory_budget(qsizetype requested_bytes) { return m_real->set_memory_budget(requested_bytes); }
qsizetype cover_provider::memory_budget() const                        { return m_real->memory_budget(); }
qsizetype cover_provider::memory_budget_min()                          { return cover_storage_LI::memory_budget_min(); }

} // namespace covers::live
