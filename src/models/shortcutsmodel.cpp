#include "shortcutsmodel.hpp"
#include "abstractmodel.hpp"
#include "defaultroles.hpp"

#include <QQmlEngine> // include here, where it's actually used

ShortcutsModel::ShortcutsModel(QObject *parent)
    : AbstractModel(parent, container_roles)
{}

ShortcutsModel *ShortcutsModel::instance() {
    // create with dinamic memory so it lives during the entire execution
    static ShortcutsModel *_instance = new ShortcutsModel();
    return _instance;
}

ShortcutsModel *ShortcutsModel::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
    ShortcutsModel *inst = instance();
    
    // CRITICAL: C++ is owner of this pointer
    // so under no circumstance this should be destroyed when application is closed.
    if (engine) {
        QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    }
    
    return inst;
}

// a shortcut could be actually anything
void ShortcutsModel::append(const Types::Any &shortcut) { AbstractModel::append(shortcut); }
void ShortcutsModel::remove(int index)                  { AbstractModel::remove(index); }
void ShortcutsModel::clear()                            { AbstractModel::clear(); }