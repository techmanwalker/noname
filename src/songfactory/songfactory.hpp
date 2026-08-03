#pragma once

#include "coverprovider.hpp"
#include "mediatypes.hpp"

#include <QFuture>
#include <QLoggingCategory>
#include <QObject>

class song_factory : public QObject {
    Q_OBJECT
public:
    // logging
    static const QLoggingCategory &l_songfactory();

    struct attributes {
        size_t crop_and_resize = 0; // if non-zero, song_factory will crop the center of the artwork and rescale it to NxN to save memory
    };

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Song> extract(const QUrl &source, std::shared_ptr<cover_provider> provider, attributes a);

    // crop_and_resize: memory saving, crop the center square of the cover and rescale so width and height matches.
    // e.g. crop_and_resize = 256: crop the center of the cover and rescale to 256x256

    static void shutdown ();

    template <typename Container>
    requires
    std::ranges::forward_range<Container> // Any type of list
    &&  (
            std::is_same_v<typename Container::value_type, QString> // any uri or path
        ||  std::is_same_v<typename Container::value_type, QUrl>)
    static
    QList<QFuture<Types::Song>>
    progressive_extract (const Container &sources, std::shared_ptr<cover_provider> provider, attributes a);

    template <typename Container>
    requires
    std::ranges::forward_range<Container> // Any type of list
    &&  (
            std::is_same_v<typename Container::value_type, QString> // any uri or path
        ||  std::is_same_v<typename Container::value_type, QUrl>)
    static
    QFuture<QList<Types::Song>>
    batch_extract (const Container &sources, std::shared_ptr<cover_provider> provider, attributes a);

private:
    // private and linear constructor
    song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Song execute_extraction(QPromise<Types::Song> &promise, attributes a);

    // Dedicated thread pool to load hundreds of songs
    static QThreadPool* extraction_pool();

    QUrl m_source;

    std::shared_ptr<cover_provider> m_cover_provider = nullptr;
};

#include "songfactory.tpp"