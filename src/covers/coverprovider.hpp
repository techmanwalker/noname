#pragma once

#include <QQuickAsyncImageProvider>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <memory>
#include <string_view>

// Forward declare the implementation to hide it from the rest of the application
namespace covers::live {
    class cover_storage;
}

namespace covers::live {

class cover_provider : public QQuickAsyncImageProvider {
public:
    explicit cover_provider(std::shared_ptr<covers::live::cover_storage> realProvider);

    // QQuickAsyncImageProvider interface
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

    // Exposed forwarding calls
    void register_source(const QString &hash, const QUrl &source, size_t crop_and_resize);
    bool store(const QString &hash, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

    // Address where cached covers are located
    static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

private:
    std::shared_ptr<covers::live::cover_storage> m_real;
};

} // namespace covers::live