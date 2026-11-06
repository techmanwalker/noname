#include "defaultroles.hpp"
#include "playlistsequence.hpp"

PlaylistSequence::PlaylistSequence(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}

void
PlaylistSequence::respawn_list (QList<Types::Song> new_list)
{
    clear();

    // this version is synchronous
    batch_append(std::move(new_list));
}