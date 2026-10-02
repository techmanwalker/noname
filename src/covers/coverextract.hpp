#pragma once

#include <QImage>

#include <taglib/fileref.h>

namespace covers::live {

// Pulls the embedded cover (if any) out of an already-open TagLib file.
QImage extract_cover (TagLib::File *file, size_t crop_and_resize);

}