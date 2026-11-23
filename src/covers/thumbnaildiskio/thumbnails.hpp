#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>

#include <stop_token>

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
// resolve_blocking, which lives as a task or runnable, invokes this
// size determines what thumbnail to look at
[[nodiscard ("Useless decoding work.")]]
QImage
fetch_thumbnail (
    const QUrl &media_source,
    QSize target_size
);

// The image of a write queued or encoding right now (null if none): the pixels the file
// will contain, usable instead of decoding the source again.
[[nodiscard]] QImage
pending_thumbnail (
    const QUrl &media_source,
    QSize target_size
);

QFuture<void>
write_thumbnail (
    const QUrl &media_source,
    QImage thumbnail,
    std::stop_token stop
);

// drop queued writes and wait for the in-flight ones; call after tripping the stop source
void
teardown ();

}
