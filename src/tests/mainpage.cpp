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

    // load qml
    QQmlApplicationEngine engine;

    // load model
    engine.rootContext()->setContextProperty("ShortcutsList", &shortcuts);

    const QUrl url = QUrl::fromLocalFile("src/tests/mainpagetest.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}