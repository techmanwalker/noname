#include "abstractmediasequence.hpp"
#include "configuration.hpp"
#include "defaultroles.hpp"
#include "songfactory.hpp"
#include "shortcutslist.hpp"

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

QFuture<void>
ShortcutsList::read_conf_and_load ()
{
    auto &conf = configuration::manager::instance();
    using configuration::conf_file_type::shortcuts;

    auto shortcuts_song_paths = conf.read_lines(shortcuts);

    return song_factory::batch_extract(shortcuts_song_paths, chosen_cover_provider)
    .then(this, [this](QList<Types::Song> loaded_shortcuts) {
        batch_append(std::move(loaded_shortcuts));
    });
}

// a shortcut could be actually anything, just not now
void ShortcutsList::append(const Types::Song &shortcut)                { AbstractMediaSequence::append(shortcut); }
void ShortcutsList::batch_append (const QList<Types::Song> &shortcuts) { AbstractMediaSequence::batch_append(shortcuts); }
void ShortcutsList::remove(size_t index)                                 { AbstractMediaSequence::remove(index); }
void ShortcutsList::clear()                                           { AbstractMediaSequence::clear(); }