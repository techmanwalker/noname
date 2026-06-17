#include "shortcutslist.hpp"
#include "abstractmediasequence.hpp"
#include "defaultroles.hpp"

#include <QQmlEngine> // include here, where it's actually used

ShortcutsList &
ShortcutsList::instance() {
    // create with dinamic memory so it lives during the entire execution
    static ShortcutsList s_instance;
    return s_instance;
}

ShortcutsList *
ShortcutsList::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);

    ShortcutsList *inst = &instance();
    
    // CRITICAL: C++ is owner of this pointer
    // so under no circumstance this should be destroyed when application is closed.
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    
    return inst;
}

ShortcutsList::ShortcutsList(QObject *parent)
    : AbstractMediaSequence(parent, container_roles)
{}

// a shortcut could be actually anything
void ShortcutsList::append(const Types::Any &shortcut)                { AbstractMediaSequence::append(shortcut); }
void ShortcutsList::batch_append (const QList<Types::Any> &shortcuts) { AbstractMediaSequence::batch_append(shortcuts); }
void ShortcutsList::remove(int index)                                 { AbstractMediaSequence::remove(index); }
void ShortcutsList::clear()                                           { AbstractMediaSequence::clear(); }