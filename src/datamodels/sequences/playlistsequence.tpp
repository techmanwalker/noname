#pragma once

#include "abstractmediasequence.hpp"
#include "defaultroles.hpp"
#include "playlistsequence.hpp"

template <media_sequence container>
PlaylistSequence::PlaylistSequence(
    container tracks,
    QObject *parent
)
    : AbstractMediaSequence(parent, container_roles)
{
    batch_append(std::move(tracks));
}

template <media_sequence container>
void
PlaylistSequence::respawn_list (container new_list)
{
    clear();

    // this version is synchronous
    batch_append(std::move(new_list));
}
