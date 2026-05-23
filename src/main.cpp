#include <QGuiApplication>
#include <QQmlApplicationEngine>

int 
main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Here we will load the main module when it's ready.
    // E.g:
    // const QUrl url(u"qrc:/startpage/src/qml/startpage/StartPage.qml"_qs);
    // engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}