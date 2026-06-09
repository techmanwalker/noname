#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
//#include <QDirIterator>

#include "lyrics/lyricsmanifest.hpp"
#include "sequences/playqueue.hpp"
#include "playbackpresentation/playbackpresentation.hpp"
#include "playbackcontroller.hpp"
#include "songfactory.hpp"

int
main (int argc, char ** argv)
{

    // Currently, just a GUI test app
    // create base application
    QGuiApplication app(argc, argv);

    // Create model
    auto& testLyrics = LyricsManifest::instance();
    
    // Populate it (example - load from file or hardcode for testing)
    testLyrics.appendLyric(0, "You shut your mouth");
    testLyrics.appendLyric(5000, "How can you say I go about things the wrong way?");
    testLyrics.appendLyric(10000, "I am human and I need to be loved");
    testLyrics.appendLyric(15000, "Just like everybody else does");

    // Playlist model
    auto& nextQueue = PlayQueue::instance();

    // Populate it (example)
    // Now with real file loading; for now, the addition will be performed synchonously before properly queuing async loading
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/apotionforlove.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/allthethingsshesaid.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/bittersweetsymphony.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/eric.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lifeinmono.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lilith.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/onlytime.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/showmehow.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/runaway.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/telephones.flac")).result());
    nextQueue.append(song_factory::extract(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/ykwim.flac")).result());

    // Dummy player test
    auto& PlaybackPresentation = PlaybackPresentation::instance();

    // Now that the manual setTitle function has vanished, now we need to formally play a song.

    playback_controller::instance().load(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/onlytime.flac"));

    // create base engine
    QQmlApplicationEngine engine;

    /*
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
    */

    // load qml
    const QUrl url = QUrl::fromLocalFile("src/tests/mockupv2.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}