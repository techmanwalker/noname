#pragma once

#include "playqueue.hpp"

#include <QFuture>
#include <QIdentityProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QUrl>

#include <memory>

class PlayQueueProxy : public QIdentityProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayQueue)
    QML_SINGLETON

    Q_PROPERTY(QModelIndex playhead READ playhead  WRITE qml_switch_to NOTIFY trackChanged)
    Q_PROPERTY(qsizetype   count    READ itemCount NOTIFY countChanged)

public:
    explicit PlayQueueProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_queue(s_injectedQueue) // Copies shared_ptr, incrementing ref count
    {
        if (m_queue) {
            setSourceModel(m_queue.get());

            connect(m_queue.get(), SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));
            connect(m_queue.get(), SIGNAL(countChanged()), this, SIGNAL(countChanged()));
        }
    }

    // LI_PlayQueue is itself a QIdentityProxyModel, not a flat QAbstractListModel like
    // LyricsManifest/LocalLibraryLDB — so this has to be typed at the common
    // QAbstractItemModel base, or setSourceModel() below wouldn't even compile.
    static void inject(const std::shared_ptr<QAbstractItemModel> &queue) {
        s_injectedQueue = queue; // Ref count incremented, caller's instance unaffected
    }

    int itemCount() const {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) return q->itemCount();
        return 0;
    }

    // Same QObject* -> interface* direction as LocalLibrary/LyricsProjector — qobject_cast
    // works fine here (unlike PlayerPresenterProxy, which needed the reverse direction).
    QPersistentModelIndex playhead() const {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) {
            QPersistentModelIndex src_idx = q->playhead();        // LI_PlayQueue's own space
            if (!src_idx.isValid()) return {};
            return QPersistentModelIndex(mapFromSource(src_idx)); // -> this proxy's space
        }
        return {};
    }

    Q_INVOKABLE void qml_switch_to(const QModelIndex &index) {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) {
            // index arrives in THIS proxy's space — map down before crossing the
            // interface, the reverse trip of playhead() above.
            q->switch_to(QPersistentModelIndex(mapToSource(index)), true);
        }
    }

    Q_INVOKABLE void switch_to(const QUrl &source) {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) q->switch_to(source);
    }

    Q_INVOKABLE void next() {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) q->next();
    }

    Q_INVOKABLE void prev() {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) q->prev();
    }

    Q_INVOKABLE void clear() {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) q->clear();
    }

    Q_INVOKABLE QFuture<void> batch_append(const QList<QUrl> &sources) {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) return q->batch_append(sources);
        return QtFuture::makeReadyVoidFuture();
    }

    Q_INVOKABLE void respawn_queue(const QStringList &sources) {
        if (auto *q = qobject_cast<PlayQueue*>(sourceModel())) q->respawn_queue(sources);
    }

signals:
    void trackChanged();
    void countChanged();

private:
    std::shared_ptr<QAbstractItemModel> m_queue;
    inline static std::shared_ptr<QAbstractItemModel> s_injectedQueue = nullptr;
};