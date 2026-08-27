#pragma once

#include <QFuture>

#include "mediatypes.hpp"

// The manager of your logical local library database
class LocalLibrary 
{

public:
    virtual ~LocalLibrary () = default;

    // request to load to memory the metadata of the songs configured in the
    // known music folders directories list
    virtual QFuture<void> snapshot_known_directories () = 0;

    virtual QList<Types::Song> flattened () const = 0;

    virtual QStringList flattened_sources () const = 0;

};

Q_DECLARE_INTERFACE(LocalLibrary, "com.noname.LocalLibrary")