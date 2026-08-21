#include "abstractmediasequence.hpp"
#include "configuration.hpp"
#include "defaultroles.hpp"
#include "songfactory.hpp" // IWYU pragma: keep
#include "shortcutslistimpl.hpp"

LI_ShortcutsList::LI_ShortcutsList(QObject *parent, std::shared_ptr<covers::live::cover_provider> provider)
    : AbstractMediaSequence(parent, container_roles),
      chosen_cover_provider(provider)
{}

QFuture<void>
LI_ShortcutsList::read_conf_and_load ()
{
    auto &conf = configuration::manager::instance();
    using configuration::conf_file_type::shortcuts;

    auto shortcuts_song_paths = conf.read_lines(shortcuts);

    return song_factory::batch_extract(shortcuts_song_paths, {256, true})
    .then(this, [this](QList<Types::Song> loaded_shortcuts) {
        for (const Types::Song &song : loaded_shortcuts) {
            chosen_cover_provider->register_cover_reference(song.cover);
        }

        batch_append(std::move(loaded_shortcuts));
    });
}

// a shortcut could be actually anything, just not now
void LI_ShortcutsList::append(const Types::Song &shortcut)                { AbstractMediaSequence::append(shortcut); }
void LI_ShortcutsList::batch_append (const QList<Types::Song> &shortcuts) { AbstractMediaSequence::batch_append(shortcuts); }
void LI_ShortcutsList::remove(size_t index)                                 { AbstractMediaSequence::remove(index); }
void LI_ShortcutsList::clear()                                           { AbstractMediaSequence::clear(); }