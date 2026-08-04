#pragma once

#include <QCache>
#include <QQuickImageProvider>
#include <QObject>
#include <qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(l_coverprovider)

// Enables loading embedded thumbnails to memory.
class cover_provider : public QQuickImageProvider
{
public:
    // Initialize indicating that this provider provides QImage objects
    cover_provider();

    // Save picture and return a generated UUID
    bool store(const QString &hash, const QVariant &cover_from_metadata, bool save_to_disk_cache = true);

    // Method called by QtQuick to retrieve te associated image
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    bool is_cached (const QString &hash);

    // Address where cached covers are (should be) located
    static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

    // Default cover image uri
    static constexpr char default_cover_uri[] = "";

private:
    QHash<QString, QImage> m_cache;
    std::atomic_flag m_spin_lock = ATOMIC_FLAG_INIT; // multithreaded insertion
};