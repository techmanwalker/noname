#pragma once

#include <QImage>

#include <taglib/fileref.h>

namespace covers::live {

// Pulls the embedded cover (if any) out of an already-open TagLib file.
QImage extract_cover (TagLib::File *file, size_t crop_and_resize);

// Percentile of OkLab lightness (L, in [0,1]) sampled from a *ring* of
// `cover`. Deliberately not the mean, for the same reason as always — the
// mean gets dragged around by large flat regions, which this exists to
// avoid. percentile is 0-100. ring_crop_begin/ring_crop_end (each 0-1)
// bound the ring by Chebyshev distance from the image's own center: 0 is
// dead center, 1 is the image's own edge (corners included, since this is
// a rectangular frame, not a circular one). [0, 1] (the defaults) samples
// the whole image; [0, 0.6] samples a solid center block; [0.8, 1] samples
// only the outer border frame. Adjacent rings never share a pixel — the
// interval is half-open on the high end ([begin, end)) except at the true
// outer edge, where end == 1 includes the literal edge pixels too.
double percentile_luminance (const QImage &cover, int percentile,
                              double ring_crop_begin = 0.0, double ring_crop_end = 1.0);

}