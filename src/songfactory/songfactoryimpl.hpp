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

class song_factory_impl : public QObject {
    Q_OBJECT
public:
    // logging
    static const QLoggingCategory &l_songfactory();

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Song> extract(const QUrl &source, std::shared_ptr<covers::live::cover_provider> provider, attributes a);

    // crop_and_resize: memory saving, crop the center square of the cover and rescale so width and height matches.
    // e.g. crop_and_resize = 256: crop the center of the cover and rescale to 256x256

    static void shutdown ();

    static
    QList<QFuture<Types::Song>>
    progressive_extract (const QList<QUrl> &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a);

    static
    QFuture<QList<Types::Song>>
    batch_extract (const QList<QUrl> &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a);

private:
    // private and linear constructor
    song_factory_impl(const QUrl &source, std::shared_ptr<covers::live::cover_provider> provider);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Song execute_extraction(QPromise<Types::Song> &promise, attributes a);

    // Dedicated thread pool to load hundreds of songs
    static QThreadPool* extraction_pool();

    QUrl m_source;

    std::shared_ptr<covers::live::cover_provider> m_cover_provider = nullptr;
};