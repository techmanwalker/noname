#pragma once

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

    // Never touch each other's instances' signals or members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Track> extract(const QUrl &source);

    static void teardown ();

    static
    QList<QFuture<Types::Track>>
    progressive_extract (const QList<QUrl> &sources);

    static
    QFuture<QList<Types::Track>>
    batch_extract (const QList<QUrl> &sources);

private:
    // private and linear constructor
    song_factory_impl(const QUrl &source);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Track execute_extraction(QPromise<Types::Track> &promise);

    // Dedicated thread pool to load hundreds of tracks
    static QThreadPool* extraction_pool();

    QUrl m_source;
};
