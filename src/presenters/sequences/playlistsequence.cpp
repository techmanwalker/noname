#include "abstractmediasequence.hpp"
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
};

void PlaylistSequence::append(const Types::Song &song) { AbstractMediaSequence::append(song); }
void PlaylistSequence::remove(size_t index)               { AbstractMediaSequence::remove(index); }
void PlaylistSequence::clear()                         { AbstractMediaSequence::clear(); }
void PlaylistSequence::items()                         { AbstractMediaSequence::items(); }

void
PlaylistSequence::batch_append(const QList<Types::Song> &songs) { AbstractMediaSequence::batch_append(songs);}

void
PlaylistSequence::respawn_list (const QList<Types::Song> &new_list)
{
    clear();

    // this version is synchronous
    batch_append(new_list);
}
