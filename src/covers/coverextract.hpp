#pragma once

#include <QImage>

#include <qsize.h>
#include <taglib/fileref.h>

namespace covers::live {

struct luma {
    double value = 0.5;
    double ring_crop_begin = 0.0;
    double ring_crop_end = 1.0;
};

struct cover_luma {
    luma nuclear_luma;
    luma centric_luma;
    luma midring_luma;
    luma borders_luma;
};

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

// P(this cover wants a light-keyed background | as many ring lumas needed),
// fit offline (logistic regression) against a hand-labeled
// centric/midring/borders survey — see /areas/noname-background-darkener
// notes. Returns a probability in [0,1]; treat it as a continuous blend
// weight between a dark-keyed and light-keyed backdrop target, not
// something to threshold to a bool — the model's own borderline cases
// (p≈0.5) are exactly where a smooth fade matters most.
double probability_light (const cover_luma &lumas);

// Area-weighted mean of the four ring lumas in `lumas`, condensed to a
// single [0,1] scalar -- the input the future dark-keyed/light-keyed
// background curves will read, replacing a single percentile_luminance
// reading. Each ring's weight is the fraction of the image's area it
// covers: exactly ring_crop_end^2 - ring_crop_begin^2, regardless of the
// cover's aspect ratio. That's not an approximation -- percentile_luminance
// normalizes each axis independently before taking the Chebyshev max, so
// the region within normalized distance s of center is always the whole
// image rectangle scaled by s about its own center, on both axes at once;
// its area is exactly s^2 of the total, no width/height needed.
// Rings that tile [0,1] edge-to-edge sum to exactly 1, making the
// normalization below a no-op -- it stays regardless, so a gap, overlap,
// or malformed ring (begin > end) degrades gracefully instead of silently
// skewing the result. Falls back to 0.5 -- the same "nothing computed
// yet" placeholder used elsewhere -- only if every ring somehow
// contributes zero area at once.
double pondered_luma (const cover_luma &lumas);

}