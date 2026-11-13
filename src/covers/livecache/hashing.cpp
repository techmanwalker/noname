#include "livecache/hashing.hpp"
#include "logging/l_coverprovider.hpp"

#include "mediatypes.hpp"

#include <QCryptographicHash>

namespace covers::hash 
{

QString
payload_and_size (
    const QByteArray &payload,
    QSize target_thumbnail_size
)
{
    // File names need to be short, alphanumeric and filesystem-safe, so the key is hashed:
    // not reversible, collisions negligible. The key itself must be injective: '|' cannot
    // occur in a base64url payload, and the size is digits around an 'x'.

    Q_ASSERT_X(isBase64Url(payload), "covers::hash::payload_and_size", 
        "payload must be unpadded base64url (see toBase64Url)");

#ifndef QT_NO_DEBUG
    // A payload is a URL encoded ONCE. If what it decodes to is base64url again, something
    // encoded it twice. Each layer is a quarter shorter, so this ends on its own.
    size_t extra_layers = 0;
    for (auto url = decodeBase64Url(payload); url && !url->isEmpty(); ) {
        url = decodeBase64Url(*url);
        if (url) ++extra_layers;
    }

    if (extra_layers > 0) {
        qCWarning(l_coverprovider) << "The payload for a hash request is base64url encoded"
            << extra_layers + 1 << "times, expected once. This is most likely a developer mistake.";
    }
#endif

    QByteArray key = 
        payload
        + "|"
        + QByteArray::number(target_thumbnail_size.width())
        + "x"
        + QByteArray::number(target_thumbnail_size.height()
    );

    return QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex());
}

}
