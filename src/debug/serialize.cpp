#include "serialize.hpp"
#include "mediatypes.hpp"

#include <QLoggingCategory>

#include <qjsonarray.h>
#include <qjsonobject.h>
#include <variant>

QJsonObject
debug::serialize (const Types::Song &song)
{
    QJsonObject out;

    out["title"] = song.title;
    out["artist"] = song.artist;
    out["album"] = song.album;
    out["duration"] = static_cast<qint64>(song.duration);
    out["source"] = song.source.toString();
    out["cover"] = song.cover.toString();

    return out;
}

QJsonObject
debug::serialize (const Types::Album &album)
{
    QJsonObject out;

    out["title"] = album.title;
    out["artist"] = album.artist;
    out["cover"] = album.cover.toString();
    out["duration"] = static_cast<qint64>(album.duration());

    QJsonArray songs;

    for (const auto &song : album.songs) {
        songs.append(serialize(song));
    }

    out["songs"] = songs;

    return out;
}

QJsonObject
debug::serialize (const Types::Directory &dir)
{
    QJsonObject out;

    out["path"] = dir.path;
    out["name"] = dir.title;

    QJsonArray songs;

    for (const auto &song : dir.songs) {
        songs.append(serialize(song));
    }

    out["songs"] = songs;

    return out;
}

QJsonObject
debug::serialize (const Types::Any &unit)
{
    QJsonObject out;

    if (std::holds_alternative<Types::Song>(unit)) {
        out = serialize(std::get<Types::Song>(unit));
    }
    if (std::holds_alternative<Types::Album>(unit)) {
        out = serialize(std::get<Types::Album>(unit));
    }

    if (std::holds_alternative<Types::Directory>(unit)) {
        out = serialize(std::get<Types::Directory>(unit));
    }

    return out;
}

QJsonArray
debug::serialize (const QList<QUrl> &uris)
{
    QJsonArray out;

    for (const auto &uri : uris) {
        out.append(uri.toString());
    }
    
    return out;
}

QJsonArray
debug::serialize (const QList<Types::Any> &media)
{
    QJsonArray out;

    for (const auto &unit : media) {
        out.append(serialize(unit));
    }

    return out;
}

void
debug::print (const QLoggingCategory &cat, const QJsonValue &val)
{
    qCDebug (cat).noquote() << val.toJson(QJsonDocument::Indented).constData();;
}