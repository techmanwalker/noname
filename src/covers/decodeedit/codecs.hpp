#pragma once

#include <QImage>

#include <cstdint>
#include <optional>
#include <stop_token>
#include <vector>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace covers::codecs {

// What the source's YUV values mean. The defaults (BT.601, full range) are the
// JPEG/MJPEG case used by embedded covers; video callers pass the AVFrame's values.
struct source_colorimetry {
    AVColorSpace space = AVCOL_SPC_BT470BG;
    AVColorRange range = AVCOL_RANGE_JPEG;
};

QImage
sws_convert_to_qimage (const uint8_t *const *src_data, const int *src_linesize,
                             int src_w, int src_h, AVPixelFormat src_fmt,
                             int dst_w, int dst_h, int flags,
                             AVPixelFormat dst_fmt, QImage::Format out_format,
                             const source_colorimetry &colorimetry = {});

// True if the local file has a real video stream (embedded cover art doesn't count).
bool has_video_stream(const QString &local_path);

// Decodes one representative picture of a local video file, at native resolution.
// Returns a null QImage if there is no real video stream or decoding fails.
QImage decode_video_frame_at_fraction(const QString &local_path, double fraction,
                                      QImage::Format out_format);

QImage decode_cover_ffmpeg(const uchar *data, size_t size, QImage::Format out_format);

QImage read_jpegxl  (const QString &path);

// No I/O. Nullopt on failure or when stop is requested, including mid-encode.
std::optional<std::vector<uint8_t>>
encode_jpegxl (const QImage &image, QImage::Format format, std::stop_token stop);

// encode_jpegxl + file write. Once an encoded buffer exists, the write always completes.
bool   write_jpegxl (const QString &path, QImage image, QImage::Format format, std::stop_token stop);

}
