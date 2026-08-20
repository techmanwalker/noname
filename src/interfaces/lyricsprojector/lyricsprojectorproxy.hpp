#pragma once

#include "lyricsprojector.hpp"
#include <QIdentityProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

class LyricsProjectorProxy : public QIdentityProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsManifest)
    QML_SINGLETON

public:
    explicit LyricsProjectorProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_manifest(s_injectedManifest) // Copies shared_ptr, incrementing ref count
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
        if (auto projector = qobject_cast<LyricsProjector*>(sourceModel())) {
            projector->clear();
        }
    }

private:
    std::shared_ptr<QAbstractListModel> m_manifest;
    inline static std::shared_ptr<QAbstractListModel> s_injectedManifest = nullptr;
};