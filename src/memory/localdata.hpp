#pragma once

#include <QImage>
#include <QLoggingCategory>
#include <QUrl>

namespace localdata {

Q_DECLARE_LOGGING_CATEGORY(l_localdata)

enum class dirs {
    thumbnails
};

QString dir();
QUrl path (dirs type);

}