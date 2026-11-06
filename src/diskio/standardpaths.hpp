#pragma once

#include <QLoggingCategory>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(l_standardpaths)

namespace standardpaths {

enum class standard_dirs : std::uint8_t {
    configuration,
    thumbnails,
    playlists
};

QString dir(standard_dirs type);

}