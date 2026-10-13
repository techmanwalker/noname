#pragma once

#include "abstractmediasequence.hpp"

#include "manager-in.hpp"
#include "shortcutslist-in.hpp"

#include <memory>

// Intended to end up showing in the Start page showing shortcuts to your favorite music.
class ShortcutsListLI : public AbstractMediaSequence, public ShortcutsList
{
    Q_OBJECT
    Q_INTERFACES(ShortcutsList)

public:
    explicit ShortcutsListLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    // must not copy nor reassign
    ShortcutsListLI(const ShortcutsListLI&) = delete;
    ShortcutsListLI& operator=(const ShortcutsListLI&) = delete;

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
    std::shared_ptr<configuration::manager> cm;
};
