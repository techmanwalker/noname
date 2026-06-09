#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
//#include <QDirIterator>

#include "lyrics/lyricsmanifest.hpp"
#include "sequences/playqueue.hpp"
#include "playbackpresentation/playbackpresentation.hpp"
#include "playbackcontroller.hpp"

int
main (int argc, char ** argv)
{

    // Currently, just a GUI test app
    // create base application
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(R"(
        qt.multimedia*=false
        qt.multimedia*.warning=true
        qt.multimedia*.critical=true
    )");

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
    // Now with concurrent batch loading: it will extract metadata in parallel 
    // and safely perform a single massive insertion on the model once ready.
    nextQueue.batch_append(QList<QUrl>{
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/apotionforlove.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/allthethingsshesaid.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/bittersweetsymphony.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/eric.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lifeinmono.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lilith.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/onlytime.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/showmehow.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/runaway.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/telephones.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/ykwim.flac")
    });

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