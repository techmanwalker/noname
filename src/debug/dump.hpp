#pragma once

#include <QLoggingCategory>
#include <QAbstractListModel>

namespace debug
{

void
dump_list_model (
    const QLoggingCategory &cat,
    const QAbstractListModel &list,
    const QString &display_list_name,
    bool dump_nested_lists = false
);

}