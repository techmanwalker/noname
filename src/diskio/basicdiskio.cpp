#include "basicdiskio.hpp"

#include <QDirIterator>

QStringList
diskio::list_dir(const QString &dir, bool recursive)
{
    QStringList file_list;

    QDirIterator::IteratorFlags it_flags = (
        recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags
    );

    QDirIterator it (dir, QDir::Files, it_flags);

    while (it.hasNext()) {
        it.next();

        file_list.append(it.fileInfo().absoluteFilePath());
    }

    return file_list;
}