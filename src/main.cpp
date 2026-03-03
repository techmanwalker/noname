#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "lyricsmodel.hpp"

int
main (int argc, char ** argv)
{
    // create base application
    QGuiApplication app(argc, argv);

    // Create model
    LyricsModel lyricsModel;
    
    // Populate it (example - load from file or hardcode for testing)
    lyricsModel.appendLyric(0, "You shut your mouth");
    lyricsModel.appendLyric(5000, "How can you say I go about things the wrong way?");
    lyricsModel.appendLyric(10000, "I am human and I need to be loved");
    lyricsModel.appendLyric(15000, "Just like everybody else does");

    // create base engine
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("lyricsModel", &lyricsModel);

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