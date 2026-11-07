#pragma once

#include <QFuture>
#include <QImage>

class cover_storage_in {

public:
    virtual ~cover_storage_in () = default;

    virtual QFuture<QImage> resolve (const QUrl &media_source, const QSize &requestedSize) = 0;

};