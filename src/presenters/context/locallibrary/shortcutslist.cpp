#include "abstractmediasequence.hpp"
#include "manager-in.hpp"
#include "defaultroles.hpp"
#include "songfactory.hpp" // IWYU pragma: keep
#include "shortcutslist.hpp"

ShortcutsListLI::ShortcutsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager)
    : AbstractMediaSequence(parent, container_roles),
      cm(confmanager)
{}

QFuture<void>
ShortcutsListLI::read_conf_and_load ()
{
    using configuration::conf_file_type::shortcuts;

    auto shortcuts_song_paths = cm->read_lines(shortcuts);

    return song_factory::batch_extract(shortcuts_song_paths, {256, true})
    .then(this, [this](QList<Types::Song> loaded_shortcuts) {
        batch_append(std::move(loaded_shortcuts));
    });
}

// a shortcut could be actually anything, just not now
void ShortcutsListLI::append(const Types::Song &shortcut)                { AbstractMediaSequence::append(shortcut); }
void ShortcutsListLI::batch_append (const QList<Types::Song> &shortcuts) { AbstractMediaSequence::batch_append(shortcuts); }
void ShortcutsListLI::remove(size_t index)                                 { AbstractMediaSequence::remove(index); }
void ShortcutsListLI::clear()                                           { AbstractMediaSequence::clear(); }