#pragma once

#include "lyricsmanifest-in.hpp"
#include "lyrictypes.hpp"

#include <QIdentityProxyModel>

#include <QtQmlIntegration//qqmlintegration.h>

#include <memory>

class LyricsManifestProxy : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsManifest)
    QML_SINGLETON

    Q_PROPERTY(QModelIndex highlighted READ index_of_first_highlighted_row NOTIFY highlightedRowChanged)
    Q_PROPERTY(qsizetype   count       READ itemCount NOTIFY countChanged)

public:

    explicit LyricsManifestProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_manifest(s_injectedManifest), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<LyricsManifest*>(m_manifest.get()))
    {
        if (m_manifest) {
            setSourceModel(m_manifest.get());
        }

        connect(m_manifest.get(), SIGNAL(highlightedRowChanged()),
                        this, SIGNAL(highlightedRowChanged()));

        connect(m_manifest.get(), SIGNAL(countChanged()), this, SIGNAL(countChanged()));
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &manifest) {
        s_injectedManifest = manifest; // Ref count incremented, caller's instance unaffected
    }

    int itemCount() const {
        if (!m_iface) return 0;
        
        return m_iface->itemCount();
    }

    // Respect and expose the public Q_INVOKABLE interface using the abstraction
    Q_INVOKABLE void clear() {
        if (!m_iface) return;

        m_iface->clear();
    }

    QModelIndex index_of_first_highlighted_row() const {
        if (!m_iface) return {};

        // Re-home the index onto this proxy — an index carries a pointer to
        // the model it belongs to, and m_iface hands back one rooted in the
        // source model, not this one.
        return mapFromSource(m_iface->index_of_first_highlighted_row());
    }

signals:
    void highlightedRowChanged();
    void countChanged();

private:
    std::shared_ptr<QAbstractListModel> m_manifest;
    inline static std::shared_ptr<QAbstractListModel> s_injectedManifest = nullptr;

    LyricsManifest *m_iface = nullptr;
};