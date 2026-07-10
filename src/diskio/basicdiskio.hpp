#pragma once

#include <QList>
#include <QLoggingCategory>

namespace diskio
{
    Q_DECLARE_LOGGING_CATEGORY(l_diskio)

    QStringList list_dir (const QString &dir, bool recursive = false);

}