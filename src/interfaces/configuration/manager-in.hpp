#pragma once

#include <QFuture>
#include <QStringList>

namespace configuration {

enum class conf_file_type {
    known_music_directories,
    shortcuts,
    window_geometry,
    volume
};

class manager
{

public:
    virtual ~manager() = default;

    /** main operations, use conf_file_type to choose
    which file's line you wish to read from or
    write to
    */
    virtual QStringList read_lines (conf_file_type type, bool unconditionally_refresh = false) = 0;

    /// its QFuture rather means when the new content has finished writing to disk
    virtual QFuture<bool> write_lines (conf_file_type type, const QStringList &lines) = 0;

};

}