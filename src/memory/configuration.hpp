#pragma once

#include <QFuture>
#include <QHash>
#include <QLoggingCategory>
#include <QReadWriteLock>
#include <QString>
#include <QUrl>

namespace configuration {

Q_DECLARE_LOGGING_CATEGORY(l_configuration)

enum class conf_file_type {
    known_music_directories,
    shortcuts,
    window_geometry
};

QUrl file (conf_file_type type);

class manager : public QObject
{
    Q_OBJECT

public:
    static manager &instance();

    // delete copy and reassignment
    manager (const manager &) = delete;
    manager &operator= (const manager &) = delete;

    /** main operations, use conf_file_type to choose
        which file's line you wish to read from or
        write to
        */
    QStringList read_lines (conf_file_type type, bool unconditionally_refresh = false);

    /// its QFuture rather means when the new content has finished writing to disk
    QFuture<bool> write_lines (conf_file_type type, const QStringList &lines);

private:
    explicit manager(QObject *parent = nullptr);
    ~manager() override = default;

    // called when lines don't exist in the cache map
    QStringList prolly_cache_lines (conf_file_type type, bool condition_to_trigger_recaching = false);
    void __cache_lines_unlocked (conf_file_type type);

    // performs the actual write to disk off thread, finishes on true if successful
    QFuture<bool> __write_lines_to_disk_unlocked (conf_file_type type, const QStringList &lines);

    mutable QReadWriteLock m_lock; // internal lock for threads within the same instance

    QHash<conf_file_type, QStringList> m_last_content_acknowledged_for_file;

};

}