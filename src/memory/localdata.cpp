#include "localdata.hpp"

#include <QDir>
#include <QStandardPaths>

namespace localdata
{

QString
dir ()
{
    // get the ~/.config path for noname
    QString conf_path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    // guarantee that it exists
    QDir().mkpath(conf_path);
    return conf_path;
}

QUrl
path (dirs type)
{
    QString dir_name;

    switch (type) {
        case dirs::thumbnails:
            dir_name = "thumbnails";
            break;
    }

    if (dir_name.isEmpty()) return QUrl();

    QDir dir_path (dir());

    return QUrl::fromLocalFile(dir_path.absoluteFilePath(dir_name));
}

}