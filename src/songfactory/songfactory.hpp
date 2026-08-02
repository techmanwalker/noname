#pragma once

#include "coverprovider.hpp"
#include "mediatypes.hpp"

#include <QFuture>
#include <QLoggingCategory>
#include <QObject>

class song_factory : public QObject {
    Q_OBJECT
public:

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Song> extract(const QUrl &source, std::shared_ptr<cover_provider> provider);

    static void shutdown ();

    template <typename Container>
    requires
    std::ranges::forward_range<Container> // Any type of list
    &&  (
            std::is_same_v<typename Container::value_type, QString> // any uri or path
        ||  std::is_same_v<typename Container::value_type, QUrl>)
    static
    QList<QFuture<Types::Song>>
    progressive_extract (const Container &sources, std::shared_ptr<cover_provider> provider);

    template <typename Container>
    requires
    std::ranges::forward_range<Container> // Any type of list
    &&  (
            std::is_same_v<typename Container::value_type, QString> // any uri or path
        ||  std::is_same_v<typename Container::value_type, QUrl>)
    static
    QFuture<QList<Types::Song>>
    batch_extract (const Container &sources, std::shared_ptr<cover_provider> provider);

private:
    // private and linear constructor
    song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Song execute_extraction(QPromise<Types::Song> &promise);

    // Dedicated thread pool to load hundreds of songs
    static QThreadPool* extraction_pool();

    QUrl m_source;

    std::shared_ptr<cover_provider> m_cover_provider = nullptr;

    // logging
    static const QLoggingCategory &l_songfactory();
};

#include "songfactory.tpp"