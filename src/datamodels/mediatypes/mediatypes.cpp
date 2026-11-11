#include "mediatypes.hpp"
#include "coveruris.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <qurl.h>

Q_LOGGING_CATEGORY(l_mediatypes, "mediatypes")

namespace {

QString
printable_time (quint64 ms)
{
    quint64 total_seconds =  ms / 1000;

    quint64 mm = total_seconds / 60;
    quint8 ss = total_seconds % 60;

    return QString::number(mm) + ":" + (ss < 10 ? "0" : "") + QString::number(ss);
}

}


QByteArray
toBase64Url (const QUrl &uri)
{
    return uri.toEncoded().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QUrl
fromBase64Url (const QByteArray &base64url)
{
    return QUrl::fromEncoded(
        QByteArray::fromBase64(base64url, QByteArray::Base64UrlEncoding)
    );
}

quint64
base_media_container_::duration() const {
    quint64 total = 0;
    for (const Types::Song &s : songs)
        total += s.duration;
    return total;
}

QString
base_media_container_::duration_mmss() const {
    return printable_time(duration());
}

QUrl
base_media_container_::firstcover() const
{
    if (songs.size() == 0) return {};

    return songs.at(0).coveruri();
}

namespace Types {
    
// validity semantics

bool
Song::is_valid () const
{
    return !source.isEmpty();
}

QUrl
Song::coveruri() const
{
    return covers::schema + source_base64();
}

QString
Song::duration_mmss () const
{
    return printable_time(duration);
}

QByteArray
Song::source_base64() const
{
    return toBase64Url(source);
}

bool
Directory::is_valid () const
{
    return !path.isEmpty();
}

}
