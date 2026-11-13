#pragma once

#include "abstractmediasequence.hpp"
#include "basicsequenceops.hpp"

#include "virtual/manager.hpp"
#include "mediatypes.hpp"
#include "virtual/playlistslist.hpp"

#include <memory>

// prolly models like this and PlaylistsList could be templated to avoid
// repeating code but somehow Qt has issues with templated QObjects :\

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class PlaylistsListLI :
    public AbstractMediaSequence,
    public BasicSequenceOps<PlaylistsListLI, Types::Playlist>, 
    public PlaylistsList
{
    Q_OBJECT
    Q_INTERFACES(PlaylistsList)

public:
    explicit PlaylistsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    USING_SEQUENCE_OPS(PlaylistsListLI, Types::Playlist);
    using AbstractMediaSequence::clear;
    QList<Types::Playlist> items () const override;

    static QString file_path_by_name (const QString &name);

    bool pointed_to (const QPersistentModelIndex &idx, std::function<bool(const Types::Playlist &)> callback) override;

    QFuture<bool> save_playlist_to_disk (const QString &name, QList<QUrl> sources) override;
    QFuture<bool> delete_playlists (const QStringList &names) override;

public slots:
    // shorthand
    QFuture<void> refresh () override;

private:
    std::shared_ptr<configuration::manager> cm;
    quint64 m_refresh_generation = 0; // bumped per refresh; stale continuations discard their results

    static QDir directory ();
};
