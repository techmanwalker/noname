#include "shortcutsmodel.hpp"
#include "abstractmodel.hpp"

ShortcutsModel::ShortcutsModel(QObject *parent)
    : AbstractModel(parent)  // delegate parent constructor
{}

// a shortcut could be actually anything
void ShortcutsModel::append(const Types::Any &shortcut) { AbstractModel::append(shortcut); }
void ShortcutsModel::remove(int index)                  { AbstractModel::remove(index); }
void ShortcutsModel::clear()                            { AbstractModel::clearItems(); }