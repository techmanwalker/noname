#pragma once

#include "attributesstruct.hpp"

#include "mediatypes.hpp"

#include <QFuture>
#include <QLoggingCategory>
#include <QObject>

// Forward declare the cover provider
namespace covers::live {
    class cover_provider;
}

class song_factory_impl : public QObject
{
    Q_OBJECT
    
public:
    // logging
    static const QLoggingCategory &l_songfactory();

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Song> extract(const QUrl &source, attributes a);
    
    static void teardown ();

    static
    QList<QFuture<Types::Song>>
    progressive_extract (const QList<QUrl> &sources, attributes a);

    static
    QFuture<QList<Types::Song>>
    batch_extract (const QList<QUrl> &sources, attributes a);

private:
    // private and linear constructor
    song_factory_impl(const QUrl &source);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Song execute_extraction(QPromise<Types::Song> &promise, attributes a);

    // Dedicated thread pool to load hundreds of songs
    static QThreadPool* extraction_pool();

    QUrl m_source;
};