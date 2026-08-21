#pragma once

#include <QIdentityProxyModel>

#include <QtQmlIntegration/qqmlintegration.h>

class ShortcutsListProxy : public QIdentityProxyModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ShortcutsList)
    QML_SINGLETON

public:
    explicit ShortcutsListProxy(QObject *parent = nullptr)
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

private:
    std::shared_ptr<QAbstractListModel> m_manifest;
    inline static std::shared_ptr<QAbstractListModel> s_injectedManifest = nullptr;
};