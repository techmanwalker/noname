#pragma once

#include <QIdentityProxyModel>

#include <QtQmlIntegration>

class ShortcutsListProxy : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ShortcutsList)
    QML_SINGLETON

public:
    explicit ShortcutsListProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_slist(s_injectedList) // Copies shared_ptr, incrementing ref count
    {
        if (m_slist) {
            setSourceModel(m_slist.get());
        }
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &list) {
        s_injectedList = list; // Ref count incremented, caller's instance unaffected
    }

private:
    std::shared_ptr<QAbstractListModel> m_slist;
    inline static std::shared_ptr<QAbstractListModel> s_injectedList = nullptr;
};