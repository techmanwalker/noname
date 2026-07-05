#pragma once

#include <QList>
#include <QLoggingCategory>
#include <QUrl>

namespace diskio
{
    Q_DECLARE_LOGGING_CATEGORY(l_diskio)

    QList<QUrl> list_dir (const QUrl dir, bool recursive = false);


}