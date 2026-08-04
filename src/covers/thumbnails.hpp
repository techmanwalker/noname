#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>

// covers::disk: thumbnails and persistent caching on non volatile media

namespace covers {
namespace disk {

Q_DECLARE_LOGGING_CATEGORY(l_thumbnails)

QString
thumbnail_hash_for (const QUrl &source, size_t crop_and_resize);

bool has_thumbnail (const QString &hash);

[[nodiscard ("Useless decoding work.")]] QImage fetch_thumbnail (const QString &hash);
QFuture<void> write_thumbnail (const QString &hash, QImage thumbnail);

}
}