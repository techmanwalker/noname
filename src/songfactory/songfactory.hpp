#pragma once
#include <QObject>
#include <QUrl>
#include <QFuture>
#include <QMediaPlayer>
#include <memory>

#include "mediatypes.hpp"
#include "coverprovider.hpp"

class song_factory : public QObject {
    Q_OBJECT
public:

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url, cover_provider);
    static QFuture<Types::Song> extract(const QUrl &source, std::shared_ptr<cover_provider> provider);

    // Enable to concurrently get metadata of songs in batches
    static QFuture<QList<Types::Song>> batch_extract(const QList<QUrl> &sources, std::shared_ptr<cover_provider> provider); // batch

private:
    // private and linear constructor
    song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider);

    // Internally executes the extraction synchronously (to be called from worker threads, cancellable promise)
    Types::Song execute_extraction(QPromise<Types::Song> &promise);

    QUrl m_source;

    std::shared_ptr<cover_provider> m_cover_provider = nullptr;
};