#pragma once

#include "mediatypes.hpp"
#include "virtual/playlistslist.hpp"
#include <QIdentityProxyModel>

#include "listproxybase.hpp"

#include <QtQmlIntegration>

// QIdentityProxyModel MUST stay the first base: the QML prototype is resolved from it
class PlaylistsListProxy : public QIdentityProxyModel,
                           public ListProxyBase<PlaylistsListProxy, PlaylistsList, Types::Playlist>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlaylistsList)
    QML_SINGLETON

    Q_PROPERTY(qsizetype count READ item_count NOTIFY countChanged)

public:
    explicit PlaylistsListProxy(QObject *parent = nullptr) : QIdentityProxyModel(parent) {
        forward(SIGNAL(count_changed()), SIGNAL(countChanged()));
    }

    Q_INVOKABLE Types::Playlist itemAt(int idx) const { return item_at(idx); }

    Q_INVOKABLE QFuture<bool> deletePlaylists(const QStringList &names) {
        return call_or(QtFuture::makeReadyValueFuture(false), &PlaylistsList::delete_playlists, names);
    }

signals:
    void countChanged();
};
