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

    qDebug() << "Lyrics appended.";

    // Playlist model
    PlaylistModel testPlaylist;

    qDebug() << "Playlist model initialized.";

    // Populate it (example)
    testPlaylist.append(Types::Song{ "A Potion For Love",  "AURORA",          "The Gods We Can Touch",                216000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/apotionforlove.jpg")             });
    qDebug() << "First song appended.";
    testPlaylist.append(Types::Song{ "All The Things She Said", "t.A.T.u",    "200 km/h in the Wrong Lane",           214000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/200_kmh.jpg")                    });
    qDebug() << "Second song appended.";
    testPlaylist.append(Types::Song{ "Bitter Sweet Symphony", "The Verve",    "Urban Hymns",                          357000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/urbanhymns.jpg")                 });
    testPlaylist.append(Types::Song{ "Eric",                  "Mitski",       "Lush",                                 197000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/lush.jpg")                       });
    testPlaylist.append(Types::Song{ "Life in Mono",          "Mono",         "Formica Blues",                        223000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/formicablues.jpg")               });
    testPlaylist.append(Types::Song{ "Lilith",                "Saint Avangeline", "Gardener of Eden",                 255000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/gardenerofeden.jpg")             });
    testPlaylist.append(Types::Song{ "Only Time",             "Enya",         "A Day Without Rain",                   218000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/adaywithoutrain.jpg")            });
    testPlaylist.append(Types::Song{ "Show Me How",           "Men I Trust",  "Show Me How",                          215000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/showmehow.jpg")                  });
    testPlaylist.append(Types::Song{ "Runaway",               "AURORA",       "All My Demons Greeting Me As A Friend",248000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/allmydemonsgreetingmeasafriend.jpg") });
    testPlaylist.append(Types::Song{ "Telephones",            "Vacations",    "Changes",                              212000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/changes.jpg")                    });
    testPlaylist.append(Types::Song{ "YKWIM?",                "Yot Club",     "Bipolar",                              212000, QUrl(), QUrl::fromLocalFile("/home/lito/Imágenes/Covers/bipolar.jpg")                    });

    qDebug() << "All songs appended.";

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