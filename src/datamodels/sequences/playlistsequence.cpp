#include "defaultroles.hpp"
#include "playlistsequence.hpp"

PlaylistSequence::PlaylistSequence(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}

PlaylistSequence::PlaylistSequence(
    QList<Types::Song> songs,
    QObject *parent
) 
    : AbstractMediaSequence(parent, container_roles)
{
    batch_append(songs);
}

void
PlaylistSequence::respawn_list (const QList<Types::Song> &new_list)
{
    clear();

    // this version is synchronous
    batch_append(new_list);
}