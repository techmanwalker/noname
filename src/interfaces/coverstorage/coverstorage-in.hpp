#pragma once

#include <QFuture>
#include <QImage>

class cover_storage {

public:
    virtual ~cover_storage () = default;

    virtual QFuture<QImage> resolve (const QUrl &media_source, const QSize &requestedSize) = 0;

};
