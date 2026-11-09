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
    // at the file system level we need short names, no bijective encoding needed here
    // whatever string comes out of here is fine as long as it does not collide
    // or is unusably long or has any non alphanumeric character

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
