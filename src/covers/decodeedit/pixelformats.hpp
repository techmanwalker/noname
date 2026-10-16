#pragma once

#include <QImage>

extern "C" {
#include <libavutil/pixfmt.h>
}

// Pixel formats used by every image decoder and loader on this player.
// Unified on this file to prevent confusion.

static constexpr QImage::Format pixelformat_qimage = QImage::Format_RGBA8888;

// Exact-layout mapping: swscale must always write the layout the QImage
// was actually sized for — never a hardcoded one.
AVPixelFormat av_format_for_qimage(QImage::Format f);