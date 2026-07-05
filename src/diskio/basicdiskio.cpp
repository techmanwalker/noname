#include "basicdiskio.hpp"

#include <QDirIterator>

QList<QUrl>
diskio::list_dir(const QUrl dir, bool recursive)
{
    QList<QUrl> file_list;

    QDirIterator::IteratorFlags it_flags = (
        recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags
    );

    if (!dir.isLocalFile()) {
        qCWarning(l_diskio) << dir << " is not a local directory. Aborting.";
        return file_list;
    }

    QDirIterator it (dir.toLocalFile(), QDir::Files, it_flags);

    while (it.hasNext()) {
        it.next();

        file_list.append(it.fileInfo().absoluteFilePath());
    }

    return file_list;
}