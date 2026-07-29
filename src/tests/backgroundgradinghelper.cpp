#include <QGuiApplication>
#include <QQmlApplicationEngine>

int
main (int argc, char ** argv)
{
    // Drop any picture here and this test will apply the Background filters for you.
    QGuiApplication app(argc, argv);

    // create base engine
    QQmlApplicationEngine engine;

    // load qml
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Player.TestQmls", "BackgroundGradingHelper.qml");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}