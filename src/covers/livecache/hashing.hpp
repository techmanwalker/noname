#pragma once

#include <QByteArray>
#include <QSize>
#include <QString>

namespace covers::hash
{

QString
payload_and_size (
    const QByteArray &payload, // for what media file
    QSize target_thumbnail_size
);

}
