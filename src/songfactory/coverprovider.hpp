#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QUuid>
#include <qimage.h>
#include <qtmetamacros.h>
#include <string_view>

// Enables loading embedded thumbnails to memory.
class cover_provider : public QQuickImageProvider
{
public:
    // Initialize indicating that this provider provides QImage objects
    cover_provider();

    // Save picture and return a generated UUID
    QString store(const QVariant &cover_from_metadata);

    // Method called by QtQuick to retrieve te associated image
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Address where cached covers are (should be) located
    static constexpr std::basic_string_view<char16_t> schema = u"image://covers/";

    // Default cover image uri
    static constexpr char default_cover_uri[] = "";

private:
    std::vector<std::pair<QString, QImage>> m_linear_cache;
    std::atomic_flag m_spin_lock = ATOMIC_FLAG_INIT; // multithreaded insertion
};