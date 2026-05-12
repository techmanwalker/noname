#include "playlistmodel.hpp"
#include "abstractmodel.hpp"
#include "defaultroles.hpp"

PlaylistModel::PlaylistModel(QObject *parent)
    : AbstractModel(parent, container_roles)
{}

void PlaylistModel::append(const Types::Song &song) { AbstractModel::append(song); }
void PlaylistModel::remove(int index)               { AbstractModel::remove(index); }
void PlaylistModel::clear()                         { AbstractModel::clear(); }