#include "shortcutsmodel.hpp"
#include "abstractmodel.hpp"
#include "defaultroles.hpp"

#include <QQmlEngine> // include here, where it's actually used

ShortcutsModel::ShortcutsModel(QObject *parent)
    : AbstractModel(parent, container_roles)
{}

ShortcutsModel *ShortcutsModel::instance() {
    // Lo creamos con memoria dinámica para que viva durante toda la ejecución
    static ShortcutsModel *_instance = new ShortcutsModel();
    return _instance;
}

ShortcutsModel *ShortcutsModel::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
    ShortcutsModel *inst = instance();
    
    // ¡CRÍTICO! Le decimos a QML que el dueño de este puntero es C++
    // y que por ningún motivo intente destruirlo al cerrar la aplicación.
    if (engine) {
        QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
    }
    
    return inst;
}

// a shortcut could be actually anything
void ShortcutsModel::append(const Types::Any &shortcut) { AbstractModel::append(shortcut); }
void ShortcutsModel::remove(int index)                  { AbstractModel::remove(index); }
void ShortcutsModel::clear()                            { AbstractModel::clear(); }