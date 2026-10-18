#pragma once

#include "abstractmediasequence.hpp"
#include "basicmediasequence.hpp"

#include "manager-in.hpp"
#include "mediatypes.hpp"
#include "playlistslist-in.hpp"

#include <memory>

// prolly models like this and PlaylistsList could be templated to avoid
// repeating code but somehow Qt has issues with templated QObjects :\

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class PlaylistsListLI : public BasicMediaSequence<Types::Playlist>, public PlaylistsList
{
    Q_OBJECT
    Q_INTERFACES(PlaylistsList)

public:
    explicit PlaylistsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    using AbstractMediaSequence::clear;

    // shorthand
    QFuture<void> read_conf_and_load ();

private:
    std::shared_ptr<configuration::manager> cm;
};
