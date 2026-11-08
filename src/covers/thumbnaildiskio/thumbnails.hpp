#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>

// covers::disk: thumbnails and persistent caching on non volatile media

namespace covers::disk {

Q_DECLARE_LOGGING_CATEGORY(l_thumbnails)

// path on disk where the thumbnail is stored and retrieved
QString
thumbnail_file_path (
    const QUrl &media_source,
    QSize target_size
);

bool 
thumbnail_file_exists(
    const QUrl &media_source,
    QSize target_size
);

// whether the file in disk actually exists
[[nodiscard ("Useless decoding work.")]]
QImage
fetch_thumbnail (
    const QUrl &media_source,
    QSize target_size
);

QFuture<void>
write_thumbnail (
    const QUrl &media_source,
    QImage thumbnail
);

}
