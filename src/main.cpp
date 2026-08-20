#include "audioengine.hpp"
#include "coverprovider.hpp"
#include "coverstorage.hpp"
#include "locallibrary.hpp"
#include "playqueue.hpp"
#include "presenters/context/playerpresenter/playerpresenter.hpp"
#include "shortcutslist.hpp"
#include "songfactory.hpp" // direct call in case we need to cancel

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <memory>
#include <qobject.h>

Q_LOGGING_CATEGORY(l_noname, "noname.app")

int
main (int argc, char ** argv)
{
    // real entry point for this player app - main executable "noname"
    // which must take the project name when I decide what it will be.

    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    // cover cache to hold the shortcuts covers across the entire session
    std::shared_ptr<covers::live::cover_storage> cover_private_storage = std::make_shared<covers::live::cover_storage>();

    // protect the cover cache from the qml gc gremlin

    /*  Explanation of the proxy mechanism:

        QQmlEngine will take ownership of this proxy, and upon program teardown,
        will invoke 'delete' on it.
        As m_real is a std::shared_ptr, when the proxy is destroyed it will
        decrement the reference counter safely without prematurely freeing the
        displaced memory block where the qml engine thinks the cover_provider is.
    */
    std::shared_ptr<covers::live::cover_provider> covers = std::make_shared<covers::live::cover_provider>(cover_private_storage);

    /*  load the songs from the known music directories and display as
        a folder-separated view of all available songs */
    auto ae = std::make_shared<audio_engine>();
    auto &ll = LocalLibrary::instance();

    auto &pq = PlayQueue::instance();
    pq.set_audio_controller(ae);

    auto &sl = ShortcutsList::instance();

    auto &pp = PlayerPresenter::instance(ae);

    // load the same cover provider in all relevant places
    ll.chosen_cover_provider = covers;
    pq.set_cover_provider(covers);
    sl.chosen_cover_provider = covers;

    // trigger first refresh
    ll.snapshot_known_directories();

    // load shortcuts
    sl.read_conf_and_load();





    // bind signals
    QObject::connect (ae.get(), &audio_engine::track_changed,
        &pq, &PlayQueue::handle_track_changed);

    QObject::connect(ae.get(), &audio_engine::queued_tracks_finished,
            &pq, &PlayQueue::handle_queued_tracks_finished);

        // Listen to the audio controller; when metadata updates, notify the QML engine
    QObject::connect(ae.get(), &audio_engine::track_changed,
            &pp, &PlayerPresenter::handleTrackChanged);

    QObject::connect(ae.get(), &audio_engine::playback_state_changed,
            &pp, &PlayerPresenter::handlePlaybackStateChanged);

    QObject::connect(ae.get(), &audio_engine::seek_finished,
            &pp, &PlayerPresenter::positionChanged);

    QObject::connect(ae.get(), &audio_engine::volume_changed,
            &pp, &PlayerPresenter::volumeChanged);

    QObject::connect (ae.get(), &audio_engine::seek_finished,
                &pp, &PlayerPresenter::gate_poll_timer);



    QTranslator translator;
    if (translator.load(QLocale::system(), "noname", "_", ":/i18n")) {
        QCoreApplication::installTranslator(&translator);
    }

    // create base engine
    QQmlApplicationEngine engine;

    // Give the QML engine its own proxy instance allocated with 'new'.
    // It will safely invoke 'delete' on this proxy without corrupting the heap
    // or interfering with the 'covers' shared_ptr used by the C++ models.
    engine.addImageProvider("covers", new covers::live::cover_provider(cover_private_storage));


    // load qml module for noname
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Player.App", "Main");

    if (engine.rootObjects().isEmpty()) {
        song_factory::teardown();
        return -1;
    }

    const int exitcode = app.exec();
    song_factory::teardown();
    ae->teardown();
    return exitcode;
}
