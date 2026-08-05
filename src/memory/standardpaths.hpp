#pragma once

#include <QLoggingCategory>
#include <QUrl>

namespace standardpaths {

Q_DECLARE_LOGGING_CATEGORY(l_localdata)

enum class standard_dirs {
    configuration,
    thumbnails
};

QString dir(standard_dirs type);

}