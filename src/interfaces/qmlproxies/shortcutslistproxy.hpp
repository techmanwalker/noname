#pragma once

#include "listproxybase.hpp"
#include <QtQmlIntegration>
#include <QIdentityProxyModel>

class ShortcutsListProxy : public QIdentityProxyModel,
                           public ListProxyBase<ShortcutsListProxy>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ShortcutsList)
    QML_SINGLETON

public:
    using QIdentityProxyModel::QIdentityProxyModel;
};
