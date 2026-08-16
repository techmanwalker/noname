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

    case standard_dirs::thumbnails:
        path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        path = QDir(path).absoluteFilePath(QStringLiteral("thumbnails"));
        break;
    }

    // Guarantee the directory exists
    QDir().mkpath(path);
    return path;
}

}