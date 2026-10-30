#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>

class cover_payload;

// covers::disk: thumbnails and persistent caching on non volatile media

namespace covers::disk {

Q_DECLARE_LOGGING_CATEGORY(l_thumbnails)

QString thumbnail_hash_for_fs (const cover_payload &ref);

// path on disk where the thumbnail is stored and retrieved
QString thumbnail_file_path   (const cover_payload &ref);

// whether the file in disk actually exists
bool    thumbnail_file_exists (const cover_payload &ref);

[[nodiscard ("Useless decoding work.")]] QImage fetch_thumbnail (const cover_payload &ref);
QFuture<void> write_thumbnail (const cover_payload &ref, QImage thumbnail);

}