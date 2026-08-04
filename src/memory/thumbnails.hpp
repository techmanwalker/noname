#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>


namespace localdata {

Q_DECLARE_LOGGING_CATEGORY(l_thumbnails)

bool has_thumbnail (const QString &hash);

[[nodiscard ("Useless decoding work.")]] QImage fetch_thumbnail (const QString &hash);
QFuture<void> write_thumbnail (const QString &hash, QImage thumbnail);

}