#include "mediatypes.hpp"
#include <qfileinfo.h>

namespace Types {

Directory::Directory (const QString &source_path)
    : path(source_path),
      title(QFileInfo(source_path).fileName())
{
}

// validity semantics

bool
Song::is_valid () const
{
    return !source.isEmpty();
}

bool
Directory::is_valid () const
{
    return !path.isEmpty();
}

}