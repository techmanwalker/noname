#include "manager-in.hpp"
#include "defaultroles.hpp"
#include "coveruris.hpp"
#include "songfactory.hpp" // IWYU pragma: keep
#include "shortcutslist.hpp"

ShortcutsListLI::ShortcutsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager)
    : AbstractMediaSequence (parent, container_roles),
      cm(confmanager)
{}

QFuture<void>
ShortcutsListLI::read_conf_and_load ()
{
    using configuration::conf_file_type::shortcuts;

    auto shortcuts_song_paths = cm->read_lines(shortcuts);

    return song_factory::batch_extract(shortcuts_song_paths, {covers::default_cover_thumbnail_size, true})
    .then(this, [this](QList<Types::Song> loaded_shortcuts) {
        batch_append(std::move(loaded_shortcuts));
    });
}