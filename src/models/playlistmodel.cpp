#include "playlistmodel.hpp"
#include "abstractmodel.hpp"

PlaylistModel::PlaylistModel(QObject *parent)
    : AbstractModel(parent)  // delegate parent constructor
{}

void PlaylistModel::append(const Types::Song &song) { AbstractModel::append(song); }
void PlaylistModel::remove(int index)               { AbstractModel::remove(index); }
void PlaylistModel::clear()                         { AbstractModel::clearItems(); }