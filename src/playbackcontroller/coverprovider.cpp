#include "coverprovider.hpp"
#include <QImage>
#include <QString>

cover_provider::cover_provider ()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QString
cover_provider::store(const QVariant &cover_from_metadata)
{
    if (!cover_from_metadata.canConvert<QImage>()) {
        return QString();
    }

    QImage img = cover_from_metadata.value<QImage>();
    if (img.isNull()) {
        return QString();
    }

    // Generamos un identificador único seguro (UUID limpio sin llaves)
    QString uid = QUuid::createUuid().toString(QUuid::Id128);
    
    m_cache.insert(uid, img);
    return uid;
}

QImage
cover_provider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    // 'id' contains the fragment that goes after "image://covers/"
    if (m_cache.contains(id)) {
        QImage img = m_cache.value(id);
        if (size) {
            *size = img.size();
        }
        return img;
    }

    // To be able to actually retrieve the default cover
    using namespace Qt::StringLiterals;

    // _s suffix creates a QString on compilation time cleanly without macros
    QImage default_cover(u":/assets/default_cover.png"_s);

    if (size) {
        *size = default_cover.size();
    }

    return default_cover;
}