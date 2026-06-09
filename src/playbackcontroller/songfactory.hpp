#pragma once
#include <QObject>
#include <QUrl>
#include <QFuture>
#include <QMediaPlayer>

#include "abstractmediasequence.hpp"
#include "coverprovider.hpp"

class song_factory : public QObject {
    Q_OBJECT
public:

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract(url);
    static QFuture<Types::Song> extract(const QUrl &source);

private:
    // private and linear constructor
    song_factory(const QUrl &source, std::shared_ptr<cover_provider> provider);

    // Internally executes the extraction synchronously (to be called from worker threads)
    Types::Song execute_extraction();

    QUrl m_source;
    std::shared_ptr<cover_provider> m_cover_provider;
};