#pragma once

#include <QQuickAsyncImageProvider>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <memory>

namespace covers::live {

// fwd
class cover_storage_LI;


class cover_provider : public QQuickAsyncImageProvider
{

public:
    explicit cover_provider(std::shared_ptr<covers::live::cover_storage_LI> realProvider);

    // QQuickAsyncImageProvider interface
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    // Exposed forwarding calls
    bool store(const QUrl &media_source, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

private:
    std::shared_ptr<covers::live::cover_storage_LI> m_real;
};



} // namespace covers::live
