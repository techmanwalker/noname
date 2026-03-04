#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "models/lyricsmodel.hpp"
#include "models/playlistmodel.hpp"

int
main (int argc, char ** argv)
{
    // create base application
    QGuiApplication app(argc, argv);

    // Create model
    LyricsModel testLyrics;
    
    // Populate it (example - load from file or hardcode for testing)
    testLyrics.appendLyric(0, "You shut your mouth");
    testLyrics.appendLyric(5000, "How can you say I go about things the wrong way?");
    testLyrics.appendLyric(10000, "I am human and I need to be loved");
    testLyrics.appendLyric(15000, "Just like everybody else does");

    // Playlist model
    PlaylistModel testPlaylist;

    // Populate it (example)
    testPlaylist.appendSong("1004 KM", "Junior H", "$ad Boyz 4 Life", 282, "", "test/covers/sadboyz4life.jpg");
    testPlaylist.appendSong("Ella", "Junior H", "Corridos tumbados", 261, "", "test/covers/corridostumbados.jpg");

    // create base engine
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LyricsModel", &testLyrics);
    engine.rootContext()->setContextProperty("NextQueue", &testPlaylist);

    // load qml
    const QUrl url = QUrl::fromLocalFile("src/qml/main.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}