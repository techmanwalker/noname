#include "basicdiskio.hpp"
#include "configuration.hpp"
#include "standardpaths.hpp"

#include <QDir>
#include <QLockFile>
#include <QStandardPaths>

#include <QtConcurrent/QtConcurrent>

namespace configuration {

Q_LOGGING_CATEGORY(l_configuration, "memory.configuration");

QString
file (setting s)
{
    QString conf_file_name;

    switch (s) {
        case setting::known_music_directories:
            conf_file_name = "music_directories";
            break;
        case setting::shortcuts:
            conf_file_name = "shortcuts";
            break;
        case setting::window_geometry:
            conf_file_name = "window_geometry";
            break;
        case setting::volume:
            conf_file_name = "volume";
    }

    // should not happen unless someone leaves an unhandled enum value
    if (conf_file_name.isEmpty()) return {};

    QDir conf_dir (standardpaths::dir(standardpaths::standard_dirs::configuration));

    return conf_dir.absoluteFilePath(conf_file_name);
}

// managerLI implementation

managerLI::managerLI(QObject *parent)
    : QObject (parent)
{
}

std::optional<QStringList>
managerLI::fetch_setting (setting s)
{
    return diskio::read_lines(file(s));
}


QFuture<bool>
managerLI::write_setting (setting s, const QStringList &lines)
{
    return diskio::write_lines(file(s), lines);
}

}
