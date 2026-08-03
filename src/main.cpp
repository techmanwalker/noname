#include "coverproviderproxy.hpp"
#include "locallibrary.hpp"
#include "playqueue.hpp"
#include "shortcutslist.hpp"
#include "songfactory.hpp" // direct call in case we need to cancel

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTranslator>

Q_LOGGING_CATEGORY(l_noname, "noname.app")

int
main (int argc, char ** argv)
{
    // real entry point for this player app - main executable "noname"
    // which must take the project name when I decide what it will be.

    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("noname"));

        // cover cache to hold the shortcuts covers across the entire session
    std::shared_ptr<cover_provider> covers = std::make_shared<cover_provider>();

    /*  load the songs from the known music directories and display as
        a folder-separated view of all available songs */
    auto &ll = LocalLibrary::instance();
    auto &pq = PlayQueue::instance();
    auto &sl = ShortcutsList::instance();

    // load the same cover provider in all relevant places
    ll.chosen_cover_provider = covers;
    pq.chosen_cover_provider = covers;
    sl.chosen_cover_provider = covers;

    // trigger first refresh
    ll.snapshot_known_directories();

    // load shortcuts
    sl.read_conf_and_load();



    QTranslator translator;
    if (translator.load(QLocale::system(), "noname", "_", ":/i18n")) {
        QCoreApplication::installTranslator(&translator);
    }

    
    // create base engine
    QQmlApplicationEngine engine;

    // protect the cover cache from the qml gc gremlin

    /*  Explanation of the proxy mechanism:

        QQmlEngine will take ownership of this proxy, and upon program teardown,
        will invoke 'delete' on it.
        As m_real is a std::shared_ptr, when the proxy is destroyed it will
        decrement the reference counter safely without prematurely freeing the
        displaced memory block where the qml engine thinks the cover_provider is.
    */
    auto *cpproxy = new __cover_provider_PROXY(covers);
    engine.addImageProvider("covers", cpproxy);


    // load qml module for noname
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Player.App", "Main");

    if (engine.rootObjects().isEmpty()) {
        song_factory::shutdown();
        return -1;
    }

    const int exitcode = app.exec();
    song_factory::shutdown();
    return exitcode;
}
