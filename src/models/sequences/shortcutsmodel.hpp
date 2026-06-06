#pragma once

#include <QtQmlIntegration/qqmlintegration.h>
#include "abstractmediasequence.hpp"

// forward declarations
class QQmlEngine;
class QJSEngine;

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class ShortcutsList : public AbstractMediaSequence {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    // must not copy nor reassign
    ShortcutsList(const ShortcutsList&) = delete;
    ShortcutsList& operator=(const ShortcutsList&) = delete;

    static ShortcutsList &instance();
    static ShortcutsList *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Accepts any type of item to be shown on shortcuts
    Q_INVOKABLE void append(const Types::Any &item);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

private:
    // hidden constructor
    explicit ShortcutsList(QObject *parent = nullptr);
};