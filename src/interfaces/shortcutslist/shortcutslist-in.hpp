#pragma once

#include <QFuture>

#include "interfaceid.hpp"

class ShortcutsList
{

public:
    virtual ~ShortcutsList () = default;


};

inline constexpr const char* ShortcutsList_iid = interface_iid<"ShortcutsList">.buf;
Q_DECLARE_INTERFACE(ShortcutsList, ShortcutsList_iid)