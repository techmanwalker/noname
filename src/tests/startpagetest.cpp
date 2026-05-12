#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "shortcutsmodel.hpp"

int
main (int argc, char ** argv)
{
    // create base application
    QGuiApplication app(argc, argv);

    // define an example shortcuts model
    ShortcutsModel shortcuts;

    shortcuts.append(Types::Song {
        "Stars",
        "t.A.T.u",
        "200 km/h in the Wrong Lane",
        249000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/200_kmh_ru.jpg")
    });

    shortcuts.append(Types::Song {
        "Not Allowed",
        "TV Girl",
        "Who Really Cares",
        167000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/whoreallycares.jpg")
    });

    shortcuts.append(Types::Song {
        "Yummy",
        "Ayesha Erotica",
        "Yummy (Righteous Remix)",
        164000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/yummy.jpg")
    });

    shortcuts.append(Types::Song {
        "Hey Kids",
        "Molina",
        "Hey Kids",
        233000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/heykids.jpg")
    });
    
    shortcuts.append(Types::Song {
        "thank u, next",
        "Ariana Grande",
        "thank u, next",
        206000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/thankunext.jpg")
    });

    shortcuts.append(Types::Song {
        "Life in Mono",
        "Mono",
        "Formica Blues",
        206000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/formicablues.jpg")
    });

    shortcuts.append(Types::Song {
        "Bang Bang Bang Bang",
        "Sohodolls",
        "Ribbed Music for the Numb Generation",
        181000,
        QUrl(),
        QUrl::fromLocalFile("/home/lito/Imágenes/Covers/ribbed.jpg")
    });

    // load qml
    QQmlApplicationEngine engine;

    // load model
    engine.rootContext()->setContextProperty("ShortcutsList", &shortcuts);

    const QUrl url = QUrl::fromLocalFile("src/tests/startpagetest.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}