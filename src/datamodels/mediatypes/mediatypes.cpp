#include "mediatypes.hpp"
#include "coveruris.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(l_mediatypes, "mediatypes")

namespace {

constexpr auto base64url_encode_options =
    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals;
constexpr auto base64url_decode_options =
    QByteArray::Base64UrlEncoding;

QString
printable_time (quint64 ms)
{
    quint64 total_seconds =  ms / 1000;

    quint64 hh = total_seconds / 3600;
    quint64 mm = total_seconds / 60 - hh * 60;
    quint8  ss = total_seconds % 60;

    return 
        (hh > 0 ? QString::number(hh) + ":" : "") +
        (mm < 10 && hh > 0 ? "0" : "") + QString::number(mm) + ":" +
        (ss < 10 ? "0" : "") + QString::number(ss);
}

}

std::optional<QByteArray>
decodeBase64Url (const QByteArray &base64url)
{
    const QByteArray bytes = QByteArray::fromBase64(base64url, base64url_decode_options);
    if (bytes.toBase64(base64url_encode_options) != base64url)
        return std::nullopt;
    return bytes;
}


QByteArray
toBase64Url (const QUrl &uri)
{
    return uri.toEncoded().toBase64(base64url_encode_options);
}

QUrl
fromBase64Url (const QByteArray &base64url)
{
    const auto bytes = decodeBase64Url(base64url);
    if (!bytes.has_value()) {
        qCWarning(l_mediatypes) << "Not a base64url payload:" << base64url;
        return QUrl();
    }
    return QUrl::fromEncoded(bytes.value());
}

bool
isBase64Url (const QByteArray &s)
{
    return decodeBase64Url(s).has_value();
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
