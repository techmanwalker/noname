#pragma once

#include "abstractmediasequence.hpp"

class PlaylistSequence : public AbstractMediaSequence {
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit PlaylistSequence(QObject *parent = nullptr);

    // only accepts songs
    Q_INVOKABLE void append(const Types::Song &song);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
};