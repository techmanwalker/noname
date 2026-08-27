#pragma once

#include "playqueue-in.hpp"

#include <QFuture>
#include <QIdentityProxyModel>
#include <QtQmlIntegration>
#include <QUrl>

#include <memory>

class PlayQueueProxy : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayQueue)
    QML_SINGLETON

    Q_PROPERTY(QModelIndex playhead READ playhead  WRITE qml_switch_to NOTIFY trackChanged)
    Q_PROPERTY(qsizetype   count    READ itemCount NOTIFY countChanged)

public:
    explicit PlayQueueProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_queue(s_injectedQueue), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<PlayQueue*>(m_queue.get()))
    {
        if (m_queue) {
            setSourceModel(m_queue.get());

            connect(m_queue.get(), SIGNAL(trackChanged()), this, SIGNAL(trackChanged()));
            connect(m_queue.get(), SIGNAL(countChanged()), this, SIGNAL(countChanged()));
        }
    }

    // PlayQueueLI is itself a QIdentityProxyModel, not a flat QAbstractListModel like
    // LyricsManifest/LocalLibraryLDB — so this has to be typed at the common
    // QAbstractItemModel base, or setSourceModel() below wouldn't even compile.
    static void inject(const std::shared_ptr<QAbstractItemModel> &queue) {
        s_injectedQueue = queue; // Ref count incremented, caller's instance unaffected
    }

    int itemCount() const {
        if (!m_iface) return 0;
        
        return m_iface->itemCount();
    }

    // Same QObject* -> interface* direction as LocalLibrary/LyricsManifest — qobject_cast
    // works fine here (unlike PlayerPresenterProxy, which needed the reverse direction).
    QPersistentModelIndex playhead() const {
        if (!m_iface) return {};

        QPersistentModelIndex src_idx = m_iface->playhead();        // PlayQueue's own space
        if (!src_idx.isValid()) return {};
        return QPersistentModelIndex(mapFromSource(src_idx)); // -> this proxy's space
    }

    Q_INVOKABLE void qml_switch_to(const QModelIndex &index) {
        if (!m_iface) return; 

        // index arrives in THIS proxy's space — map down before crossing the
        // interface, the reverse trip of playhead() above.
        m_iface->switch_to(QPersistentModelIndex(mapToSource(index)), true);
    }

    Q_INVOKABLE void switch_to(const QUrl &source) {
        if (!m_iface) return; 
        m_iface->switch_to(source);
    }

    Q_INVOKABLE void next() {
        if (!m_iface) return;
        m_iface->next();
    }

    Q_INVOKABLE void prev() {
        if (!m_iface) return; 
        m_iface->prev();
    }

    Q_INVOKABLE void clear() {
        if (!m_iface) return; 
        m_iface->clear();
    }

    Q_INVOKABLE QFuture<void> batch_append(const QList<QUrl> &sources) {
        if (!m_iface) return QtFuture::makeReadyVoidFuture(); 
        return m_iface->batch_append(sources);
    }

    Q_INVOKABLE void respawn_queue(const QStringList &sources) {
        if (!m_iface) return;
        m_iface->respawn_queue(sources);
    }

signals:
    void trackChanged();
    void countChanged();

private:
    std::shared_ptr<QAbstractItemModel> m_queue;
    inline static std::shared_ptr<QAbstractItemModel> s_injectedQueue = nullptr;

    PlayQueue *m_iface = nullptr;
};