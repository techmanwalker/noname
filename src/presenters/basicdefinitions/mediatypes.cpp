#include "mediatypes.hpp"
#include <qfileinfo.h>

Types::Directory::Directory (const QString &source_path)
    : path(source_path),
      title(QFileInfo(source_path).fileName())
{
}