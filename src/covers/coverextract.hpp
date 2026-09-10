#pragma once

#include <QImage>

#include <taglib/fileref.h>

namespace covers::live {

// Pulls the embedded cover (if any) out of an already-open TagLib file.
QImage extract_cover (TagLib::File *file, size_t crop_and_resize);

// Percentile of OkLab lightness (L, in [0,1]) across `cover`. Deliberately
// not the mean — the mean gets dragged around by large flat regions (a
// bright white border, a black letterbox bar), which is exactly the failure
// mode this exists to avoid. percentile is 0-100. center_inset (0-1) trims
// that fraction off *each* edge before sampling — e.g. 0.25 samples only
// the central 50% of the image; 0.0 (default) samples the whole image.
double percentile_luminance (const QImage &cover, int percentile, double center_inset = 0.0);

}