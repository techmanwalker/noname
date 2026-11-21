#include "serialize.hpp"
#include "mediatypes.hpp"

#include <QLoggingCategory>

QJsonObject
debug::serialize (const Types::Track &track)
{
    QJsonObject out;

    out["title"] = track.title;
    out["artist"] = track.artist;
    out["album"] = track.album;
    out["duration"] = static_cast<qint64>(track.duration);
    out["source"] = track.source.toString();
    out["cover"] = track.coveruri().toString();

    return out;
}

QJsonObject
debug::serialize (const Types::Album &album)
{
    QJsonObject out;

    out["title"] = album.title;
    out["artist"] = album.artist;
    out["duration"] = static_cast<qint64>(album.duration());

    QJsonArray tracks;

    for (const auto &track : album.tracks) {
        tracks.append(serialize(track));
    }

    out["tracks"] = tracks;

    return out;
}

QJsonObject
debug::serialize (const Types::Directory &dir)
{
    QJsonObject out;

    out["path"] = dir.path;
    out["name"] = dir.title;

    QJsonArray tracks;

    for (const auto &track : dir.tracks) {
        tracks.append(serialize(track));
    }

    out["tracks"] = tracks;

    return out;
}

QJsonArray
debug::serialize (const std::vector<std::string> &lines)
{
    QJsonArray out;

    for (const auto &line : lines) {
        out.append(QString::fromStdString(line));
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

void
debug::print (const QLoggingCategory &cat, const QJsonValue &val)
{
    qCDebug (cat).noquote() << val.toJson(QJsonDocument::Indented).constData();;
}
