#pragma once

#include <QQuickAsyncImageProvider>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <memory>

class CoverRef;

// Forward declare the implementation to hide it from the rest of the application
namespace covers::live {
    class cover_storage;
}

namespace covers::live {

class cover_provider : public QQuickAsyncImageProvider
{

public:
    explicit cover_provider(std::shared_ptr<covers::live::cover_storage> realProvider);

    // QQuickAsyncImageProvider interface
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    // Exposed forwarding calls
    bool store(const CoverRef &ref, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

private:
    std::shared_ptr<covers::live::cover_storage> m_real;
};

} // namespace covers::live