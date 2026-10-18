#include "standardpaths.hpp"

#include <QDir>
#include <QStandardPaths>

namespace standardpaths
{

QString
dir (standard_dirs type)
{
    QString path;

    switch (type) {
    case standard_dirs::configuration:
        path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        break;

    // The directory where covers caches its thumbnails
    case standard_dirs::thumbnails:
        path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        path = QDir(path).absoluteFilePath(QStringLiteral("thumbnails"));
        break;

    // The directory containing the playlists. Every plain text file with a list of paths
    // counts as a playlist.
    case standard_dirs::playlists:
        path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        path = QDir(path).absoluteFilePath(QStringLiteral("playlists"));
        break;
    }

    // Guarantee the directory exists
    QDir().mkpath(path);
    return path;
}

}