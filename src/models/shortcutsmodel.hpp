#pragma once

#include "abstractmodel.hpp"

class ShortcutsModel : public AbstractModel {
    Q_OBJECT
public:
    explicit ShortcutsModel(QObject *parent = nullptr);

    // Accepts any type of item to be shown on shortcuts
    Q_INVOKABLE void append(const Types::Any &item);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();
};