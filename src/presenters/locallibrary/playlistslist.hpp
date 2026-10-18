#pragma once

#include "abstractmediasequence.hpp"

#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playlistslist-in.hpp"

#include <memory>

// prolly models like this and PlaylistsList could be templated to avoid
// repeating code but somehow Qt has issues with templated QObjects :\

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class PlaylistsListLI : public AbstractMediaSequence, public PlaylistsList
{
    Q_OBJECT
    Q_INTERFACES(PlaylistsList)

public:
    explicit PlaylistsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    // must not copy nor reassign
    PlaylistsListLI(const PlaylistsListLI&) = delete;
    PlaylistsListLI& operator=(const PlaylistsListLI&) = delete;

    void append(const Types::Playlist &item);
    void batch_append ( )= delete; // not needed
    void remove(size_t index);
    void clear();

    // shorthand
    QFuture<void> read_conf_and_load ();

private:
    std::shared_ptr<configuration::manager> cm;
};
