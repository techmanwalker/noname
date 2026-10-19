#pragma once

#include "abstractmediasequence.hpp"
#include "basicsequenceops.hpp"

#include <QObject>

// AbstractMediaSequence MUST stay the first base: the QML prototype is resolved from it
class PlaylistSequence : public AbstractMediaSequence,
                         public BasicSequenceOps<PlaylistSequence, Types::Song>
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    explicit PlaylistSequence(
        QList<Types::Song>  songs,
        QObject            *parent = nullptr
    );

    // both bases declare these names, so the leaf picks the typed ones
    USING_SEQUENCE_OPS(PlaylistSequence, Types::Song);

    using AbstractMediaSequence::clear;

    /// clear and repopulate this playlist in one step
    void respawn_list (const QList<Types::Song> &new_list);
};
