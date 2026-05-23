#pragma once

#include <QtQmlIntegration/qqmlintegration.h>
#include "abstractmodel.hpp"

// forward declarations
class QQmlEngine;
class QJSEngine;

class ShortcutsModel : public AbstractModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ShortcutsList)
    QML_SINGLETON
public:
        // must not copy nor reassign
    ShortcutsModel(const ShortcutsModel&) = delete;
    ShortcutsModel& operator=(const ShortcutsModel&) = delete;

    static ShortcutsModel &instance();
    static ShortcutsModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Accepts any type of item to be shown on shortcuts
    Q_INVOKABLE void append(const Types::Any &item);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

private:
    // hidden constructor
    explicit ShortcutsModel(QObject *parent = nullptr);
};