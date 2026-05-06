#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
//#include <QDirIterator>

#include "lyricsmodel.hpp"
#include "playlistmodel.hpp"
#include "playerstate.hpp"

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
    testPlaylist.append("A Potion For Love", "AURORA", "The Gods We Can Touch", 216, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/apotionforlove.jpg"));
    testPlaylist.append("All The Things She Said", "t.A.T.u", "200 km/h in the Wrong Lane", 214, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/200_kmh.jpg"));
    testPlaylist.append("Bitter Sweet Symphony", "The Verve", "Urban Hymns", 357, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/urbanhymns.jpg"));
    testPlaylist.append("Eric", "Mitski", "Lush", 197, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/lush.jpg"));
    testPlaylist.append("Life in Mono", "Mono", "Formica Blues", 223, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/formicablues.jpg"));
    testPlaylist.append("Lilith", "Saint Avangeline", "Gardener of Eden", 255, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/gardenerofeden.jpg"));
    testPlaylist.append("Only Time", "Enya", "A Day Without Rain", 218, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/adaywithoutrain.jpg"));
    testPlaylist.append("Show Me How", "Men I Trust", "Show Me How", 215, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/showmehow.jpg"));
    testPlaylist.append("Runaway", "AURORA", "All My Demons Greeting Me As A Friend", 248, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/allmydemonsgreetingmeasafriend.jpg"));
    testPlaylist.append("Telephones", "Vacations", "Changes", 212, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/changes.jpg"));
    testPlaylist.append("YKWIM?", "Yot Club", "Bipolar", 212, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/bipolar.jpg"));

    // Dummy player test
    PlayerState playerState;

    // Test player state
    playerState.setTitle("Eric");
    playerState.setArtist("Mitski");
    playerState.setAlbum("Lush");
    playerState.setCover(QUrl::fromLocalFile("/home/lito/Imágenes/Covers/lush.jpg"));
    playerState.setDuration_ms(197000);
    playerState.setPosition_ms(147000);

    // create base engine
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LyricsModel", &testLyrics);
    engine.rootContext()->setContextProperty("NextQueue", &testPlaylist);
    engine.rootContext()->setContextProperty("Player", &playerState);

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