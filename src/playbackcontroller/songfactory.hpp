#pragma once
#include <QObject>
#include <QUrl>
#include <QAudioDecoder>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <functional>
#include <memory>

#include "abstractmediasequence.hpp"
#include "coverprovider.hpp"

class song_factory : public QObject {
    Q_OBJECT
public:
    // What happens when the song data is finally processed?
    using then = std::function<void(const Types::Song&)>;

    // Never touches each other's instances' signals nor members
    // Invoked as song_factory::extract
    static void extract(const QUrl &source, then callback);

private:
    song_factory(const QUrl &source, then callback, std::shared_ptr<cover_provider> provider);

    void start();

    void handle_media_status_changed(QMediaPlayer::MediaStatus status);

    void handle_error();

    QUrl m_source;
    then m_callback;
    QMediaPlayer m_media_player; // exclusively dedicated to extract the audio metadata

    std::shared_ptr<cover_provider> m_cover_provider;
};