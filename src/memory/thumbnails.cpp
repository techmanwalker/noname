#include "localdata.hpp"
#include "thumbnails.hpp"

#include <QDir>
#include <QFile>
#include <QThreadPool>

#include <QtConcurrent/QtConcurrent>

#include <ktx.h>

#include <algorithm>
#include <thread>

namespace localdata {

Q_LOGGING_CATEGORY(l_localdata, "noname.memory.localdata")

namespace { // anonymous

/*  RAII wrapper around a ktxTexture2*. Every path below (bad input, encode
    failure, write failure) needs the texture destroyed exactly once; this
    guard does that instead of duplicating cleanup at each early return. */
class KtxTexture2Guard
{
public:
    KtxTexture2Guard () = default;
    KtxTexture2Guard (const KtxTexture2Guard &) = delete;
    KtxTexture2Guard &operator= (const KtxTexture2Guard &) = delete;

    ~KtxTexture2Guard ()
    {
        if (m_texture) ktxTexture2_Destroy(m_texture);
    }

    // Matches the `ktxTexture2 **newTex` out-parameter signature shared by
    // every ktxTexture2_Create*() function.
    ktxTexture2 **operator& () { return &m_texture; }

    ktxTexture2 *get () const { return m_texture; }
    ktxTexture2 *operator-> () const { return m_texture; }

private:
    ktxTexture2 *m_texture = nullptr;
};

/*  ktxTextureCreateInfo::vkFormat is a plain ktx_uint32_t (see ktx.h) — libktx
    never actually needs a real VkFormat/<vulkan/vulkan.h> dependency for this.
    Value is VK_FORMAT_R8G8B8A8_SRGB from the stable core Vulkan 1.0 spec, kept
    as a named constant here instead of pulling in a header this file has no
    other use for. */
constexpr ktx_uint32_t k_vk_format_r8g8b8a8_srgb = 43;

// path(dirs::thumbnails) is the *directory*; this resolves the per-hash file
// inside it and guarantees the directory itself exists before use.
QString
thumbnail_file_path (const QString &hash)
{
    QDir thumb_dir (path(dirs::thumbnails).toLocalFile());
    thumb_dir.mkpath(".");
    return thumb_dir.absoluteFilePath(hash + QStringLiteral(".ktx2"));
}

// thumbnail encode+write thread pool; uastc is heavy
QThreadPool *
thumbnail_pool ()
{
    static QThreadPool pool;

    return &pool;
}

// The actual encode + disk write. Now purely an implementation detail —
// the public write_thumbnail() below only ever queues this onto
// thumbnail_pool(), it's never called directly.
bool
write_thumbnail_blocking (const QString &hash, const QImage &thumbnail)
{
    if (thumbnail.isNull()) {
        qCWarning(l_localdata) << "refusing to write a null thumbnail for" << hash;
        return false;
    }

    const QImage rgba = thumbnail.format() == QImage::Format_RGBA8888
        ? thumbnail
        : thumbnail.convertToFormat(QImage::Format_RGBA8888);

    const int width = rgba.width();
    const int height = rgba.height();
    const qsizetype tight_row_bytes = qsizetype(width) * 4;

    // libktx expects a tightly packed buffer (no per-row padding). QImage's
    // bytesPerLine() can exceed width * 4 depending on platform/alignment,
    // so repack row-by-row rather than assume they match.
    QByteArray packed;
    packed.resize(tight_row_bytes * height);
    for (int y = 0; y < height; ++y) {
        std::memcpy(packed.data() + y * tight_row_bytes,
                     rgba.constScanLine(y),
                     tight_row_bytes);
    }

    ktxTextureCreateInfo create_info = {};
    create_info.vkFormat = k_vk_format_r8g8b8a8_srgb;
    create_info.baseWidth = static_cast<ktx_uint32_t>(width);
    create_info.baseHeight = static_cast<ktx_uint32_t>(height);
    create_info.baseDepth = 1;
    create_info.numDimensions = 2;
    create_info.numLevels = 1;
    create_info.numLayers = 1;
    create_info.numFaces = 1;
    create_info.isArray = KTX_FALSE;
    create_info.generateMipmaps = KTX_FALSE;

    KtxTexture2Guard texture;
    ktx_error_code_e result = ktxTexture2_Create(
        &create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    if (result != KTX_SUCCESS) {
        qCWarning(l_localdata) << "failed to create ktx texture for" << hash
                                << ktxErrorString(result);
        return false;
    }

    result = ktxTexture_SetImageFromMemory(
        ktxTexture(texture.get()), 0, 0, 0,
        reinterpret_cast<const ktx_uint8_t *>(packed.constData()),
        static_cast<ktx_size_t>(packed.size()));
    if (result != KTX_SUCCESS) {
        qCWarning(l_localdata) << "failed to set thumbnail image data for" << hash
                                << ktxErrorString(result);
        return false;
    }

    ktxBasisParams params = {};
    params.structSize = sizeof(params);
    params.codec = KTX_BASIS_CODEC_UASTC_LDR_4x4;
    params.uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT;
    params.threadCount = std::max(1u, std::thread::hardware_concurrency());

    result = ktxTexture2_CompressBasisEx(texture.get(), &params);
    if (result != KTX_SUCCESS) {
        qCWarning(l_localdata) << "failed to uastc-encode thumbnail for" << hash
                                << ktxErrorString(result);
        return false;
    }

    const QString file_path = thumbnail_file_path(hash);
    result = ktxTexture2_WriteToNamedFile(texture.get(), qUtf8Printable(file_path));
    if (result != KTX_SUCCESS) {
        qCWarning(l_localdata) << "failed to write thumbnail" << file_path
                                << ktxErrorString(result);
        return false;
    }

    return true;
}

} // anonymous


bool
has_thumbnail (const QString &hash)
{
    return QFile::exists(thumbnail_file_path(hash));
}

// Read the file ~/.local/share/noname/thumbnails/<hash> and decode it using ktx2
QImage
fetch_thumbnail (const QString &hash)
{
    const QString file_path = thumbnail_file_path(hash);

    if (!has_thumbnail(hash)) return QImage();

    KtxTexture2Guard texture;
    ktx_error_code_e result = ktxTexture2_CreateFromNamedFile(
        qUtf8Printable(file_path),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &texture);

    if (result != KTX_SUCCESS) {
        qCWarning(l_localdata) << "failed to open thumbnail" << file_path
                                << ktxErrorString(result);
        return QImage();
    }

    if (ktxTexture2_NeedsTranscoding(texture.get())) {
        result = ktxTexture2_TranscodeBasis(texture.get(), KTX_TTF_RGBA32, 0);
        if (result != KTX_SUCCESS) {
            qCWarning(l_localdata) << "failed to transcode thumbnail" << file_path
                                    << ktxErrorString(result);
            return QImage();
        }
    }

    const ktx_uint32_t row_pitch = ktxTexture_GetRowPitch(ktxTexture(texture.get()), 0);
    const ktx_uint8_t *pixels = ktxTexture_GetData(ktxTexture(texture.get()));

    if (!pixels) {
        qCWarning(l_localdata) << "thumbnail has no image data after decode" << file_path;
        return QImage();
    }

    /*  QImage keeps only a shallow reference to the buffer we pass in, and
       `texture` is destroyed when this function returns, so force a deep
       copy now rather than let it dangle. */
    return QImage(pixels, texture->baseWidth, texture->baseHeight,
                  row_pitch, QImage::Format_RGBA8888).copy();
};

QFuture<void>
write_thumbnail (const QString &hash, QImage thumbnail)
{
    return QtConcurrent::run(thumbnail_pool(), [hash, thumbnail] {
        write_thumbnail_blocking(hash, thumbnail);
    });
}

void
shutdown ()
{
    QThreadPool *pool = thumbnail_pool();
    pool->clear();          // drop anything queued but not yet started
    pool->waitForDone();    // let whatever's already encoding finish naturally
}

}