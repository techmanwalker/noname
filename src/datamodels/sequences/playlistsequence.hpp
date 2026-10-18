#pragma once

#include "abstractmediasequence.hpp"
#include "basicmediasequence.hpp"

#include <QObject>

class PlaylistSequence : public BasicMediaSequence<Types::Song>
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    explicit PlaylistSequence(
        QList<Types::Song>  songs,
        QObject            *parent = nullptr
    );

    using AbstractMediaSequence::clear;

    /// clear and repopulate this playlist in one step
    void respawn_list (const QList<Types::Song> &new_list);
};
