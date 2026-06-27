#pragma once

#include "abstractmediasequence.hpp"

#include <QtQmlIntegration/qqmlintegration.h>

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

    // Intended to any type of item to be shown on shortcuts
    // but noname currently only supports raw songs, will be undone later when
    // actual album support is added
    void append(const Types::Song &item);
    void batch_append (const QList<Types::Song> &item);
    void remove(int index);
    void clear();

    // Shortcuts can not only be songs, so this has no respawn functions

private:
    // hidden constructor
    explicit ShortcutsList(QObject *parent = nullptr);
};
