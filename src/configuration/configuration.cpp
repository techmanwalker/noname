#include "configuration.hpp"

#include <QDir>
#include <QLockFile>
#include <QStandardPaths>

#include <QtConcurrent/QtConcurrent>
#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qreadwritelock.h>

namespace configuration {

Q_LOGGING_CATEGORY(l_configuration, "noname.configuration");

QString
dir ()
{
    // get the ~/.config path for noname
    QString conf_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    // guarantee that it exists
    QDir().mkpath(conf_path);
    return conf_path;
}

QUrl
file (conf_file_type type)
{
    QString conf_file_name;

    switch (type) {
        case conf_file_type::known_music_directories:
            conf_file_name = "music_directories";
            break;
        case conf_file_type::shortcuts:
            conf_file_name = "shortcuts";
            break;
    }

    // should not happen unless someone leaves an unhandled enum value
    if (conf_file_name.isEmpty()) return QUrl();

    QDir conf_dir (dir());

    return QUrl::fromLocalFile(conf_dir.filePath(conf_file_name));
}

// manager implementation

manager::manager(QObject *parent)
    : QObject (parent)
{
}

manager &
manager::instance()
{
    static manager inst;
    return inst;
}

QStringList
manager::read_lines (conf_file_type type, bool unconditionally_refresh)
{

    if (unconditionally_refresh) {
        return prolly_cache_lines (type, true); // unconditional, recache no matter what
    }

    // quick try to read before, flow stops here if file was already cached
    {
        QReadLocker locker(&m_lock);
        if (m_last_content_acknowledged_for_file.contains(type)) {
            return m_last_content_acknowledged_for_file.value(type);
        }

        // this locker is destroyed here, hence why scoped in braces
    }

    // if file was not present in cache, verify again with the trigger condition
    // and safely trigger caching if not met
    return prolly_cache_lines (type, !m_last_content_acknowledged_for_file.contains(type));
}

QStringList
manager::prolly_cache_lines (conf_file_type type, bool condition_to_trigger_recaching)
{
    // externally lock for reading + obbey a simple recache trigger
    QWriteLocker locker (&m_lock);
    if (condition_to_trigger_recaching) {
        __cache_lines_unlocked(type);
    }

    // in both cases, fetch whatever is in cache
    return m_last_content_acknowledged_for_file.value(type);
}

void
manager::__cache_lines_unlocked (conf_file_type type)
{
    // unsafe to use as-is, read write locks must be externally managed

    QUrl file_uri = file(type); // file path of conf file to read
    if (file_uri.isEmpty()) return;

    QString local_path = file_uri.toLocalFile();

    QStringList lines;

    QFile file_object (local_path);

    if (!file_object.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; // if it does not exist, safely return empty
    }

    QTextStream in (&file_object);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (!line.isEmpty()) {
            lines.append(line);
        }
    }

    // insert or overwrite, does not matter
    m_last_content_acknowledged_for_file.insert(type, lines);

    // qCInfo(l_configuration) << "File " << local_path << "was reloaded. Manually refresh or restart the program for changes to take effect.";
}

QFuture<bool>
manager::write_lines (conf_file_type type, const QStringList &lines)
{
    // immediately update cache, tryLock may last up to 10s but this is a write operation so nevermind
    QWriteLocker locker (&m_lock);

    // but the data write to disk still needs to be done off thread
    auto future = __write_lines_to_disk_unlocked(type, lines);

    // alter cache only if disk write succeeded
    future.then(this, [this, type, lines](bool did_write_finish_successfully) {
        QWriteLocker locker (&m_lock);

        if (did_write_finish_successfully) {
            // we are already here, update the cached lines
            m_last_content_acknowledged_for_file.insert(type, lines);
        } else {
            // read whatever the other instance wrote 
            __cache_lines_unlocked(type);
        }
    });

    return future;
}

QFuture<bool>
manager::__write_lines_to_disk_unlocked (conf_file_type type, const QStringList &lines)
{
    // no direct access to the "this" in case it dies

    auto deferred_write = [](conf_file_type type, const QStringList lines) -> bool {
        QUrl file_uri = file(type);
        if (file_uri.isEmpty()) return false;

        QString local_path = file_uri.toLocalFile();

        // lock file to sync multiple instances of the player
        QLockFile lock_file (local_path + QStringLiteral(".lock"));

        // try to acquire lock
        if (!lock_file.tryLock(10000)) {
            qCWarning(configuration::l_configuration) << "Failed to write to " << local_path << " because the file was locked for writing for more than 10 seconds.";
            return false;
        }

        QFile file_object (local_path);

        if (!file_object.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qCWarning(configuration::l_configuration) << "Failed to open " << local_path << " for writing. Aborting.";
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

    return QtConcurrent::run(deferred_write, type, lines);
}

}