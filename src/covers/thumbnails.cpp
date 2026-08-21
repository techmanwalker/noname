#include "mediatypes.hpp"
#include "pixelformats.hpp"
#include "standardpaths.hpp"
#include "thumbnails.hpp"

#include <QDir>
#include <QFile>
#include <QThreadPool>

#include <QtConcurrent/QtConcurrent>

#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner_cxx.h>

namespace covers {

namespace disk {

namespace { // anonymous

// path(dirs::thumbnails) is the *directory*; this resolves the per-hash file
// inside it and guarantees the directory itself exists before use.

// thumbnail encode+write thread pool; uastc is heavy
QThreadPool *
thumbnail_pool ()
{
    static QThreadPool pool;

    return &pool;
}

/*  The actual encode + disk write. Now purely an implementation detail —
    the public write_thumbnail() below only ever queues this onto
    thumbnail_pool(), it's never called directly. */
bool
write_thumbnail_blocking (const CoverRef &ref, const QImage &thumbnail)
{
    if (thumbnail.isNull()) {
        qCWarning(l_standardpaths) << "refusing to write a null thumbnail for" << ref.hash();
        return false;
    }

    const QImage rgba = thumbnail.format() == pixelformat_qimage
        ? thumbnail
        : thumbnail.convertToFormat(pixelformat_qimage);

    const int width  = rgba.width();
    const int height = rgba.height();
    const size_t pixels = size_t(width) * size_t(height);
    const size_t bytes  = pixels * 4;

    // Tightly packed RGBA8888 buffer
    std::vector<uint8_t> packed(bytes);
    for (int y = 0; y < height; ++y) {
        std::memcpy(packed.data() + y * width * 4,
                    rgba.constScanLine(y),
                    size_t(width) * 4);
    }

    auto enc = JxlEncoderMake(nullptr);
    if (!enc) {
        qCWarning(l_standardpaths) << "failed to create JxlEncoder for" << ref.hash();
        return false;
    }

    auto runner = JxlResizableParallelRunnerMake(nullptr);
    if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
            JxlResizableParallelRunner, runner.get()))
    {
        qCWarning(l_standardpaths) << "failed to set parallel runner for" << ref.hash();
        return false;
    }

    JxlBasicInfo basic_info;
    JxlEncoderInitBasicInfo(&basic_info);
    basic_info.xsize = width;
    basic_info.ysize = height;
    basic_info.bits_per_sample = 8;
    basic_info.alpha_bits = 8;
    basic_info.num_color_channels = 3;
    basic_info.num_extra_channels = 1;          // alpha
    basic_info.uses_original_profile = JXL_FALSE;

    if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
        qCWarning(l_standardpaths) << "JxlEncoderSetBasicInfo failed for" << ref.hash();
        return false;
    }

    JxlColorEncoding color_encoding;
    JxlColorEncodingSetToSRGB(&color_encoding, /*is_gray=*/JXL_FALSE);
    if (JXL_ENC_SUCCESS != JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
        qCWarning(l_standardpaths) << "JxlEncoderSetColorEncoding failed for" << ref.hash();
        return false;
    }

    JxlEncoderFrameSettings *frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

    // Requested settings
    JxlEncoderSetFrameDistance(frame_settings, 1.0f);                 // distance = 1
    JxlEncoderFrameSettingsSetOption(frame_settings,
                                     JXL_ENC_FRAME_SETTING_EFFORT, 7); // effort = 7

    JxlPixelFormat pixel_format = {
        4,                      // num_channels (RGBA)
        JXL_TYPE_UINT8,
        JXL_NATIVE_ENDIAN,
        0                       // align
    };

    if (JXL_ENC_SUCCESS != JxlEncoderAddImageFrame(frame_settings,
                                                   &pixel_format,
                                                   packed.data(),
                                                   bytes))
    {
        qCWarning(l_standardpaths) << "JxlEncoderAddImageFrame failed for" << ref.hash();
        return false;
    }

    JxlEncoderCloseInput(enc.get());

    std::vector<uint8_t> compressed;
    compressed.resize(64 * 1024);   // start with 64 KiB, grow as needed

    uint8_t *next_out = compressed.data();
    size_t avail_out = compressed.size();

    JxlEncoderStatus status = JXL_ENC_NEED_MORE_OUTPUT;
    while (status == JXL_ENC_NEED_MORE_OUTPUT) {
        status = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);

        if (status == JXL_ENC_NEED_MORE_OUTPUT) {
            size_t offset = next_out - compressed.data();
            compressed.resize(compressed.size() * 2);
            next_out = compressed.data() + offset;
            avail_out = compressed.size() - offset;
        }
    }

    if (status != JXL_ENC_SUCCESS) {
        qCWarning(l_standardpaths) << "JxlEncoderProcessOutput failed for" << ref.hash();
        return false;
    }

    compressed.resize(next_out - compressed.data());

    const QString file_path = thumbnail_file_path(ref);
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(l_standardpaths) << "failed to open thumbnail for writing" << file_path
                                << file.errorString();
        return false;
    }

    if (file.write(reinterpret_cast<const char*>(compressed.data()),
                   qint64(compressed.size())) != qint64(compressed.size()))
    {
        qCWarning(l_standardpaths) << "failed to write JPEG XL data" << file_path
                                << file.errorString();
        file.remove();
        return false;
    }

    return true;
}

} // anonymous

QString thumbnail_file_path (const CoverRef &ref)
{
    if (ref.hash().isEmpty()) return QString();

    QDir thumb_dir (dir(standardpaths::standard_dirs::thumbnails));
    thumb_dir.mkpath(".");
    return thumb_dir.absoluteFilePath(ref.hash() + QStringLiteral(".jxl"));
}

bool thumbnail_file_exists(const CoverRef &ref)
{
    return QFile::exists(thumbnail_file_path(ref));
}

// Read the file ~/.local/share/noname/thumbnails/<hash>.tga and decode it
QImage
fetch_thumbnail (const CoverRef &ref)
{
    const QString file_path = thumbnail_file_path(ref);

    if (!thumbnail_file_exists(ref))
        return QImage();

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(l_standardpaths) << "failed to open thumbnail" << file_path
                                << file.errorString();
        return QImage();
    }

    const QByteArray compressed = file.readAll();
    if (compressed.isEmpty()) {
        qCWarning(l_standardpaths) << "empty JPEG XL file" << file_path;
        return QImage();
    }

    auto dec = JxlDecoderMake(nullptr);
    if (!dec) {
        qCWarning(l_standardpaths) << "failed to create JxlDecoder for" << file_path;
        return QImage();
    }

    auto runner = JxlResizableParallelRunnerMake(nullptr);
    if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
            JxlResizableParallelRunner, runner.get()))
    {
        qCWarning(l_standardpaths) << "failed to set decoder parallel runner" << file_path;
        return QImage();
    }

    if (JXL_DEC_SUCCESS != JxlDecoderSubscribeEvents(dec.get(),
            JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE))
    {
        qCWarning(l_standardpaths) << "JxlDecoderSubscribeEvents failed" << file_path;
        return QImage();
    }

    JxlDecoderSetInput(dec.get(),
                       reinterpret_cast<const uint8_t*>(compressed.constData()),
                       size_t(compressed.size()));
    JxlDecoderCloseInput(dec.get());

    JxlBasicInfo info;
    JxlPixelFormat format = {4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

    std::vector<uint8_t> pixels;
    bool got_image = false;

    for (;;) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

        if (status == JXL_DEC_ERROR) {
            qCWarning(l_standardpaths) << "JPEG XL decoder error" << file_path;
            return QImage();
        }
        if (status == JXL_DEC_NEED_MORE_INPUT) {
            qCWarning(l_standardpaths) << "JPEG XL needs more input (truncated?)" << file_path;
            return QImage();
        }
        if (status == JXL_DEC_BASIC_INFO) {
            if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &info)) {
                qCWarning(l_standardpaths) << "JxlDecoderGetBasicInfo failed" << file_path;
                return QImage();
            }
            if (info.xsize == 0 || info.ysize == 0) {
                qCWarning(l_standardpaths) << "invalid dimensions in" << file_path;
                return QImage();
            }
        }
        else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            size_t buffer_size = 0;
            if (JXL_DEC_SUCCESS != JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
                qCWarning(l_standardpaths) << "JxlDecoderImageOutBufferSize failed" << file_path;
                return QImage();
            }

            pixels.resize(buffer_size);
            if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                               pixels.data(), buffer_size))
            {
                qCWarning(l_standardpaths) << "JxlDecoderSetImageOutBuffer failed" << file_path;
                return QImage();
            }
        }
        else if (status == JXL_DEC_FULL_IMAGE) {
            got_image = true;
        }
        else if (status == JXL_DEC_SUCCESS) {
            break;
        }
    }

    if (!got_image || pixels.empty()) {
        qCWarning(l_standardpaths) << "no image decoded from" << file_path;
        return QImage();
    }

    // Force deep copy
    QImage image(pixels.data(),
                 int(info.xsize), int(info.ysize),
                 int(info.xsize) * 4,
                 pixelformat_qimage);

    return image.copy();
}

QFuture<void>
write_thumbnail (const CoverRef &ref, QImage thumbnail)
{
    return QtConcurrent::run(thumbnail_pool(), [ref, thumbnail] {
        write_thumbnail_blocking(ref, thumbnail);
    });
}

void
teardown ()
{
    QThreadPool *pool = thumbnail_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already encoding finish naturally
}

}

}