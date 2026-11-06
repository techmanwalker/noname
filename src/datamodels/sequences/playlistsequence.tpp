#pragma once

#include "abstractmediasequence.hpp"
#include "defaultroles.hpp"
#include "playlistsequence.hpp"

template <media_sequence container>
PlaylistSequence::PlaylistSequence(
    container songs,
    QObject *parent
) 
    : AbstractMediaSequence(parent, container_roles)
{
    batch_append(std::move(songs));
}