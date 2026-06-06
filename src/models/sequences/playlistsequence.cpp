#include "playlistsequence.hpp"
#include "abstractmediasequence.hpp"
#include "defaultroles.hpp"

PlaylistSequence::PlaylistSequence(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}

void PlaylistSequence::append(const Types::Song &song) { AbstractMediaSequence::append(song); }
void PlaylistSequence::remove(int index)               { AbstractMediaSequence::remove(index); }
void PlaylistSequence::clear()                         { AbstractMediaSequence::clear(); }