#pragma once

#include <QImage>
#include <QUrl>

#include <taglib/fileref.h>

namespace covers::live {

// Pulls the embedded cover (if any) out of an already-open TagLib file.
QImage extract_cover (TagLib::File *file);

// Take a frame of a video to generate its display cover
QImage make_thumbnail (const QUrl &file);

}