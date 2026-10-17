#include "basicdiskio.hpp"

#include <QDirIterator>

namespace diskio
{

QStringList
list_dir (const QString &dir, bool recursive)
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

std::optional<QStringList>
read_lines (const QString &file)
{
    QStringList lines;

    QFile file_object (file);

    if (!file_object.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt; // if it does not exist, safely return empty
    }

    QTextStream in (&file_object);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (!line.isEmpty()) {
            lines.append(line);
        }
    }

    return lines;
}

}