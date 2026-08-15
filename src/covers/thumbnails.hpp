#pragma once

#include <QFuture>
#include <QImage>
#include <QLoggingCategory>
#include <QString>

class CoverRef;

// covers::disk: thumbnails and persistent caching on non volatile media

namespace covers {
namespace disk {

Q_DECLARE_LOGGING_CATEGORY(l_thumbnails)

[[nodiscard ("Useless decoding work.")]] QImage fetch_thumbnail (const CoverRef &ref);
QFuture<void> write_thumbnail (const CoverRef &ref, QImage thumbnail);

}
}