#pragma once

#include "abstractmediasequence.hpp"

#include "basicmediasequence.hpp"
#include "manager-in.hpp"
#include "shortcutslist-in.hpp"

#include <memory>

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class ShortcutsListLI : public BasicMediaSequence<Types::Song>, public ShortcutsList
{
    Q_OBJECT
    Q_INTERFACES(ShortcutsList)

public:
    explicit ShortcutsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    // Intended to any type of item to be shown on shortcuts
    // but noname currently only supports raw songs, will be undone later when
    // actual album support is added

    using AbstractMediaSequence::clear;

    // shorthand
    QFuture<void> read_conf_and_load ();

private:
    std::shared_ptr<configuration::manager> cm;
};
