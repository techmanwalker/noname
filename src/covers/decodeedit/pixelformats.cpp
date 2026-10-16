#include "pixelformats.hpp"

AVPixelFormat av_format_for_qimage(QImage::Format f)
{
    switch (f) {
    case QImage::Format_RGBA8888:   return AV_PIX_FMT_RGBA;
    case QImage::Format_RGB888:     return AV_PIX_FMT_RGB24;
    case QImage::Format_Grayscale8: return AV_PIX_FMT_GRAY8;
    default:                        return AV_PIX_FMT_NONE;
    }
}