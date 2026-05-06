#include <QGuiApplication>
#include <QQmlApplicationEngine>

int
main (int argc, char ** argv)
{
    // create base application
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // load qml
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