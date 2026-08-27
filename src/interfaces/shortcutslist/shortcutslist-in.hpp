#pragma once

#include <QFuture>

class ShortcutsList
{

public:
    virtual ~ShortcutsList () = default;


};

Q_DECLARE_INTERFACE(ShortcutsList, "com.noname.ShortcutsList")
