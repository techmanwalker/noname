#include <QQuickImageProvider>
#include "coverprovider.hpp"

// --- Cheats so the QQmlEngine doesn't hard kill the player every time it's closed
class __cover_provider_PROXY : public QQuickImageProvider {
public:
    __cover_provider_PROXY(std::shared_ptr<cover_provider> realProvider);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
private:
    std::shared_ptr<cover_provider> m_real;
};