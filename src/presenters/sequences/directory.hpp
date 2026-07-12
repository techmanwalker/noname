#pragma once

#include "abstractmediasequence.hpp" // for l_mediasequences
#include "basicdiskio.hpp"
#include "coverprovider.hpp"
#include "mediatypes.hpp"
#include "songfactory.hpp"

#include <QFileInfo>
#include <QFuture>
#include <QHash>
#include <QString>
#include <QStringList>

namespace Types {

class Directory {

public:
    explicit Directory(const QString &path, std::shared_ptr<cover_provider> coverprovider);

    ~Directory() = default;

    QString m_path;

    QFuture<QList<Types::Song>> songs();

    void refresh_cache();
    QStringList children_paths () const;
    QString name() const;
    QString path() const;

    // where are covers cached?
    std::shared_ptr<cover_provider> chosen_cover_provider;

private:
    QHash<QString, Types::Song> m_song_cache;
    QStringList m_children_file_paths_cache;
};

}