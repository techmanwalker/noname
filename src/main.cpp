#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "models/lyricsmodel.hpp"
#include "models/playlistmodel.hpp"
#include "models/playerstate.hpp"

int
main (int argc, char ** argv)
{

    // Currently, just a GUI test app
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
    testPlaylist.append("Обезьянка ноль", "t.A.T.u", "Люди Инвалиды", 265, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/liudi.jpg"));
    testPlaylist.append("Stars", "t.A.T.u", "200 km/h in the Wrong Lane", 247, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/200_kmh_ru.jpg"));

    // Dummy player test
    PlayerState playerState;

    // Test player state
    playerState.setTitle("How Soon Is Now");
    playerState.setArtist("t.A.T.u");
    playerState.setAlbum("200 km/h in the Wrong Lane");
    playerState.setCover(QUrl::fromLocalFile("/home/lito/Imágenes/Covers/200_kmh.jpg"));
    playerState.setDuration_ms(195000);
    playerState.setPosition_ms(0);

    // create base engine
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LyricsModel", &testLyrics);
    engine.rootContext()->setContextProperty("NextQueue", &testPlaylist);
    engine.rootContext()->setContextProperty("Player", &playerState);

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