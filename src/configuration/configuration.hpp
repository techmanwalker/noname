#pragma once

#include <QFuture>
#include <QHash>
#include <QLoggingCategory>
#include <QReadWriteLock>
#include <QString>
#include <QUrl>

#include "manager-in.hpp"

namespace configuration {

Q_DECLARE_LOGGING_CATEGORY(l_configuration)

QUrl file (conf_file_type type);

class managerLI : public QObject, public manager
{
    Q_OBJECT

public:
    explicit managerLI(QObject *parent);
    ~managerLI() override = default;

    QStringList read_lines (conf_file_type type, bool unconditionally_refresh = false) override;

    /// its QFuture rather means when the new content has finished writing to disk
    QFuture<bool> write_lines (conf_file_type type, const QStringList &lines) override;

private:

    // called when lines don't exist in the cache map
    QStringList prolly_cache_lines (conf_file_type type, bool condition_to_trigger_recaching = false);
    void __cache_lines_unlocked (conf_file_type type);

    // performs the actual write to disk off thread, finishes on true if successful
    QFuture<bool> __write_lines_to_disk_unlocked (conf_file_type type, const QStringList &lines);

    mutable QReadWriteLock m_lock; // internal lock for threads within the same instance

    QHash<conf_file_type, QStringList> m_last_content_acknowledged_for_file;

};

}