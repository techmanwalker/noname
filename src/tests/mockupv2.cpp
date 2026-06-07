#include <QGuiApplication>
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
    nextQueue.append(Types::Song{ "A Potion For Love",  "AURORA",          "The Gods We Can Touch",                216000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/apotionforlove.jpg")                 });
    nextQueue.append(Types::Song{ "All The Things She Said", "t.A.T.u",    "200 km/h in the Wrong Lane",           214000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/200_kmh.jpg")                        });
    nextQueue.append(Types::Song{ "Bitter Sweet Symphony", "The Verve",    "Urban Hymns",                          357000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/urbanhymns.jpg")                     });
    nextQueue.append(Types::Song{ "Eric",                  "Mitski",       "Lush",                                 197000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/lush.jpg")                           });
    nextQueue.append(Types::Song{ "Life in Mono",          "Mono",         "Formica Blues",                        223000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/formicablues.jpg")                   });
    nextQueue.append(Types::Song{ "Lilith",                "Saint Avangeline", "Gardener of Eden",                 255000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/gardenerofeden.jpg")                 });
    nextQueue.append(Types::Song{ "Only Time",             "Enya",         "A Day Without Rain",                   218000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/adaywithoutrain.jpg")                });
    nextQueue.append(Types::Song{ "Show Me How",           "Men I Trust",  "Show Me How",                          215000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/showmehow.jpg")                      });
    nextQueue.append(Types::Song{ "Runaway",               "AURORA",       "All My Demons Greeting Me As A Friend",248000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/allmydemonsgreetingmeasafriend.jpg") });
    nextQueue.append(Types::Song{ "Telephones",            "Vacations",    "Changes",                              212000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/changes.jpg")                        });
    nextQueue.append(Types::Song{ "YKWIM?",                "Yot Club",     "Bipolar",                              212000, QUrl(), QUrl::fromLocalFile("/home/notangel/Imágenes/Covers/bipolar.jpg")                        });

    // Dummy player test
    auto& PlaybackPresentation = PlaybackPresentation::instance();

    // Now that the manual setTitle function has vanished, now we need to formally play a song.

    playback_controller::instance().load(QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/genesis.flac"));

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