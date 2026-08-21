#pragma once

#include "abstractmediasequence.hpp"
#include "coverprovider.hpp"
#include "shortcutslist.hpp"

#include <QtQmlIntegration/qqmlintegration.h>

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class LI_ShortcutsList : public AbstractMediaSequence, public ShortcutsList
{
    Q_OBJECT
    Q_INTERFACES(ShortcutsList)

public:
    explicit LI_ShortcutsList(QObject *parent, std::shared_ptr<covers::live::cover_provider> provider);

    // must not copy nor reassign
    LI_ShortcutsList(const LI_ShortcutsList&) = delete;
    LI_ShortcutsList& operator=(const LI_ShortcutsList&) = delete;

    // Intended to any type of item to be shown on shortcuts
    // but noname currently only supports raw songs, will be undone later when
    // actual album support is added
    void append(const Types::Song &item);
    void batch_append (const QList<Types::Song> &item);
    void remove(size_t index);
    void clear();

    // shorthand
    QFuture<void> read_conf_and_load ();

private:

    std::shared_ptr<covers::live::cover_provider> chosen_cover_provider;
};
