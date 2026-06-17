#include "mediatypes.hpp"

#include <QJsonArray>
#include <QJsonObject>

namespace debug {
    QJsonObject serialize(const Types::Song &song);
    QJsonObject serialize(const Types::Album &album);
    QJsonObject serialize(const Types::Any &unit);

    QJsonArray serialize(const QList<QUrl> &uris);
    QJsonArray serialize(const QList<Types::Any> &media);
}