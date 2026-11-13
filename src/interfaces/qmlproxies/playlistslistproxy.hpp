#pragma once

#include "mediatypes.hpp"
#include "virtual/playlistslist.hpp"
#include <QIdentityProxyModel>

#include <QtQmlIntegration>

class PlaylistsListProxy : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlaylistsList)
    QML_SINGLETON

    Q_PROPERTY (int count READ item_count NOTIFY countChanged)

public:
    explicit PlaylistsListProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_slist(s_injectedList), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<PlaylistsList*>(m_slist.get()))
    {
        if (m_slist) {
            setSourceModel(m_slist.get());

            connect(m_slist.get(), SIGNAL(count_changed()), this, SIGNAL(countChanged()));
        }
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &list) {
        s_injectedList = list; // Ref count incremented, caller's instance unaffected
    }

    int item_count () {
        return s_injectedList->rowCount();
    }

    Q_INVOKABLE Types::Playlist itemAt (int idx) {
        if (!m_iface) return {};

        Types::Playlist result{};
        m_iface->pointed_to(mapToSource(index(idx, 0)), [&result](const Types::Playlist &playlist_at_idx) {
            result = playlist_at_idx;
            return true;
        });

        return result;
    }

    Q_INVOKABLE QFuture<bool> deletePlaylists (const QStringList &names) {
        if (!m_iface) return QtFuture::makeReadyValueFuture(false);

        return m_iface->delete_playlists(names);
    }
signals:
    void countChanged();

private:
    std::shared_ptr<QAbstractListModel> m_slist;
    inline static std::shared_ptr<QAbstractListModel> s_injectedList = nullptr;

    PlaylistsList *m_iface = nullptr;
};
