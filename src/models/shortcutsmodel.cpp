#include "shortcutsmodel.hpp"
#include "abstractmodel.hpp"
#include "defaultroles.hpp"

ShortcutsModel::ShortcutsModel(QObject *parent)
    : AbstractModel(parent, container_roles)
{}

// a shortcut could be actually anything
void ShortcutsModel::append(const Types::Any &shortcut) { AbstractModel::append(shortcut); }
void ShortcutsModel::remove(int index)                  { AbstractModel::remove(index); }
void ShortcutsModel::clear()                            { AbstractModel::clear(); }