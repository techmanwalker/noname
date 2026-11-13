#include "basicdiskio.hpp"

#include <QDirIterator>
#include <QtConcurrent/QtConcurrent>

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
read_lines (const QString &path)
{
    QStringList lines;

    QFile file_object (path);

    if (!file_object.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt; // if it does not exist, safely return empty
    }

    QTextStream in (&file_object);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (!line.isEmpty()) {
            lines.append(std::move(line));
        }
    }

    return lines;
}

QFuture<bool>
write_lines(const QString &path, QStringList lines)
{
    auto deferred_write = [](QString local_path, QStringList lines) -> bool {

        // lock file to sync multiple instances of the player
        QLockFile lock_file (local_path + QStringLiteral(".lock"));

        // try to acquire lock
        if (!lock_file.tryLock(10000)) {
            qCWarning(l_diskio) << "Failed to write to " << local_path << " because the file was locked for writing for more than 10 seconds.";
            return false;
        }

        QFile file_object (local_path);

        if (!file_object.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qCWarning(l_diskio) << "Failed to open " << local_path << " for writing. Aborting.";
            lock_file.unlock();
            return false;
        }

        QTextStream out(&file_object);
        for (const QString& line : lines) {
            out << line << "\n";
        }

        file_object.close();
        lock_file.unlock(); // free external lock

        return true;
    };

    return QtConcurrent::run(deferred_write, path, std::move(lines));
}

bool
move_to_trash(const QString &path)
{
    QFile file(path);
    if (!file.exists() || !QFile::supportsMoveToTrash()) {
        return false;
    }
    
    if (!file.moveToTrash()) {
        qCWarning (l_diskio) << "Failed to move file to trash:" << file.errorString();
        return false;
    }

    return true;
}

}
