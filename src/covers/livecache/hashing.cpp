#include "livecache/hashing.hpp"

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
