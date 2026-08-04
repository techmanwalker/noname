#include "coverprovider.hpp"

// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
class __cover_provider_PROXY : public QQuickAsyncImageProvider {
public:
    __cover_provider_PROXY(std::shared_ptr<covers::live::cover_provider> realProvider);

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
private:
    std::shared_ptr<covers::live::cover_provider> m_real;
};