#pragma once

#include <QImage>

extern "C" {
#include <libavutil/pixfmt.h>
}

// Pixel formats used by every image decoder and loader on this player.
// Unified on this file to prevent confusion.

static constexpr QImage::Format pixelformat_qimage = QImage::Format_RGBA8888;
static constexpr AVPixelFormat  pixelformat_ffmpeg = AV_PIX_FMT_RGBA;