#pragma once

#include "lyricsmanifest-in.hpp"
#include <QIdentityProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

class LyricsManifestProxy : public QIdentityProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsManifest)
    QML_SINGLETON

public:
    explicit LyricsManifestProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_manifest(s_injectedManifest), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<LyricsManifest*>(m_manifest.get()))
    {
        if (m_manifest) {
            setSourceModel(m_manifest.get());
        }
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &manifest) {
        s_injectedManifest = manifest; // Ref count incremented, caller's instance unaffected
    }

    // Respect and expose the public Q_INVOKABLE interface using the abstraction
    Q_INVOKABLE void clear() {
        if (!m_iface) return;

        m_iface->clear();
    }

private:
    std::shared_ptr<QAbstractListModel> m_manifest;
    inline static std::shared_ptr<QAbstractListModel> s_injectedManifest = nullptr;

    LyricsManifest *m_iface = nullptr;
};