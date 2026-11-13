#pragma once

#include "virtual/playqueue.hpp"

#include <QFuture>
#include <QIdentityProxyModel>
#include <QtQmlIntegration>
#include <QUrl>

#include "listproxybase.hpp"

#include <memory>

class PlayQueueProxy
    : public QIdentityProxyModel,
      public ListProxyBase<PlayQueueProxy, PlayQueue, Types::Song, &PlayQueue::item_count>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayQueue)
    QML_SINGLETON

    Q_PROPERTY(QModelIndex playhead READ playhead  WRITE qml_switch_to NOTIFY trackChanged)
    Q_PROPERTY(qsizetype   count    READ item_count NOTIFY countChanged)

    Q_PROPERTY(bool shuffle READ shuffled WRITE set_shuffle NOTIFY shuffleChanged)

public:
    explicit PlayQueueProxy(QObject *parent = nullptr) : QIdentityProxyModel (parent) {
        forward(SIGNAL(track_changed()),   SIGNAL(trackChanged()));
        forward(SIGNAL(count_changed()),   SIGNAL(countChanged()));
        forward(SIGNAL(shuffle_changed()), SIGNAL(shuffleChanged()));
    }

    // mapFromSource() already maps an invalid index to an invalid one
    QPersistentModelIndex playhead() const {
        return QPersistentModelIndex(mapFromSource(call(&PlayQueue::playhead)));
    }

    bool shuffled() const { return call(&PlayQueue::shuffled); }
    void set_shuffle(bool enabled) { call(&PlayQueue::set_shuffle, enabled); }

    void qml_switch_to(const QModelIndex &index) {
        if (!iface()) return;

        // index arrives in THIS proxy's space — map down before crossing the
        // interface, the reverse trip of playhead() above.
        iface()->switch_to(QPersistentModelIndex(mapToSource(index)), true);
    }

    Q_INVOKABLE QModelIndex findBySource(const QString &needle) {
        return mapFromSource(call(&PlayQueue::find_by_source, needle));
    }

    Q_INVOKABLE QFuture<void> batchAppend(const QList<QUrl> &sources) {
        return call_or(QtFuture::makeReadyVoidFuture(), &PlayQueue::batch_append, sources);
    }

    Q_INVOKABLE void respawnQueue(const QList<Types::Song> &songs) {
        if (!iface()) return;
        iface()->respawn_queue(songs);
    }

    Q_INVOKABLE void respawnQueue(const QStringList &sources) {
        if (!iface()) return;
        iface()->respawn_queue(sources);
    }

    Q_INVOKABLE void remove (std::vector<int> indices) {
        if (!iface()) return;

        QList<QPersistentModelIndex> qpindices;
        qpindices.reserve(static_cast<qsizetype>(indices.size()));

        for (int row : indices) {
            const QModelIndex src = mapToSource(index(row, 0));
            if (!src.isValid()) continue;
            qpindices.append(QPersistentModelIndex(src));
        }

        if (qpindices.isEmpty()) return;
        iface()->batch_remove(qpindices);
    }

    Q_INVOKABLE Types::Song itemAt(int idx) const { return item_at(idx); }

    // rows arrive in THIS proxy's space; map down before crossing the interface
    Q_INVOKABLE bool move (int from, int to) {
        if (!iface()) return false;

        const QModelIndex src  = mapToSource(index(from, 0));
        const QModelIndex dest = mapToSource(index(to, 0));
        if (!src.isValid() || !dest.isValid()) return false;

        return iface()->move_item(QPersistentModelIndex(src), QPersistentModelIndex(dest));
    }

    Q_INVOKABLE void saveToDisk () {
        if (!iface()) return;

        // currently there is no way to input the name
        iface()->save_as_playlist_to_disk(QUuid::createUuid().toString(QUuid::WithoutBraces));
    }

signals:
    void trackChanged();
    void countChanged();
    void shuffleChanged();

public slots:
    void switchTo(const QUrl &source) {
        if (!iface()) return;
        iface()->switch_to(source);
    }

    bool next()  { return call(&PlayQueue::next); }
    bool prev()  { return call(&PlayQueue::prev, false); }
    void clear() { call(&PlayQueue::clear); }
};
