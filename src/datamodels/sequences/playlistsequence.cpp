#include "defaultroles.hpp"
#include "playlistsequence.hpp"

PlaylistSequence::PlaylistSequence(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}
