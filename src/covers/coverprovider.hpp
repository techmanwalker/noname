#pragma once

#include <QCache>
#include <QLoggingCategory>
#include <QQuickImageProvider>

Q_DECLARE_LOGGING_CATEGORY(l_coverprovider)

// covers::live: hot cache on memory right away to consume, already decoded and yet but cheap to decode

namespace covers {
namespace live {

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
    QCache<QString, QImage> m_cache;

    /*  guards ALL cache access, not just insertion — QCache can evict (delete)
        an entry from any thread during insert(), so unlocked reads are no longer
        safe like they were with QHash */
    std::atomic_flag m_spin_lock = ATOMIC_FLAG_INIT; 
};

}
}