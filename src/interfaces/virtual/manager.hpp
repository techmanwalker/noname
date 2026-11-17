#pragma once

#include <QFuture>
#include <QStringList>

namespace configuration {

enum class setting {
    known_music_directories,
    shortcuts,
    window_geometry,
    volume
};

class manager
{

public:
    virtual ~manager() = default;

    virtual std::optional<QStringList> fetch_setting (setting s) = 0;

    // its QFuture means when the new content has finished writing to disk
    virtual QFuture<bool> write_setting (setting s, const QStringList &lines) = 0;

};

}
