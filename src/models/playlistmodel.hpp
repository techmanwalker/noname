#pragma once

#include "abstractmodel.hpp"

class PlaylistModel : public AbstractModel {
    Q_OBJECT
public:
    explicit PlaylistModel(QObject *parent = nullptr);

    // Solo acepta Songs
    Q_INVOKABLE void append(const Types::Song &song);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
};