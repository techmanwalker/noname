#include "audioengine.hpp"
#include "configuration.hpp"
#include "coverprovider.hpp"
#include "coverstorage.hpp"
#include "interfaces/configuration/windowgeometryproxy.hpp"
#include "interfaces/searchresults/searchresultsproxy.hpp"
#include "locallibrary.hpp"
#include "locallibraryproxy.hpp"
#include "lyricsmanifestproxy.hpp"
#include "playerpresenterproxy.hpp"
#include "playerpresenter.hpp"
#include "lyricsmanifest.hpp"
#include "playqueue.hpp"
#include "playqueueproxy.hpp"
#include "searchresults.hpp"
#include "shortcutslist.hpp"
#include "shortcutslistproxy.hpp"
#include "songfactory.hpp" // direct call in case we need to cancel
#include "windowgeometry.hpp"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <memory>

Q_LOGGING_CATEGORY(l_noname, "noname.app")

int
main (int argc, char ** argv)
{
    // real entry point for this player app main executable
    // which must take the project name when I decide what it will be.

    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    // cover cache to hold the shortcuts covers across the entire session
    std::shared_ptr<covers::live::cover_storage> cover_private_storage = std::make_shared<covers::live::cover_storage>();

    /*  
        Composition root: instantiate concrete implementations behind each
        interface boundary. Order reflects the dependency graph (pq needs ae;
        pn needs ae+pq+lm). Ownership is shared_ptr only, to avoid
        double-ownership with Qt's parent/child teardown, hence all initialized
        with nullptr.
    */
    auto cm = std::make_shared<configuration::managerLI> (nullptr);

    auto ae = std::make_shared<audio_engineLI>    (nullptr);
    auto ll = std::make_shared<LocalLibraryLI>    (nullptr, cm);
    auto lm = std::make_shared<LyricsManifestLI>  (nullptr);
    auto pq = std::make_shared<PlayQueueLI>       (nullptr, ae);
    auto pp = std::make_shared<PlayerPresenterLI> (nullptr, cm, ae, pq);
    auto sl = std::make_shared<ShortcutsListLI>   (nullptr, cm);
    auto sr = std::make_shared<SearchResultsLI>   (nullptr);
    auto wi = std::make_shared<WindowGeometryLI>  (nullptr, cm);

    /*  load the songs from the known music directories and display as
        a folder-separated view of all available songs */
    ll->snapshot_known_directories();

    // load shortcuts
    sl->read_conf_and_load();

    // initialize QML proxies
    LyricsManifestProxy::inject(lm);
    LocalLibraryProxy::inject(ll);
    PlayerPresenterProxy::inject(pp);
    PlayQueueProxy::inject(pq);
    ShortcutsListProxy::inject(sl);
    SearchResultsProxy::inject(sr);
    WindowGeometryProxy::inject(wi);


    // bind signals
    QObject::connect (ae.get(), &audio_engineLI::track_changed,
        pq.get(), &PlayQueueLI::handle_track_changed);

    QObject::connect(ae.get(), &audio_engineLI::queued_tracks_finished,
            pq.get(), &PlayQueueLI::handle_queued_tracks_finished);

    // Listen to the audio controller; when metadata updates, notify the UI's data.
    // These stay here, not in the proxy: both ae and pn are concrete pointers, and
    // main.cpp is the one place allowed to wire concrete-to-concrete connections.
    QObject::connect(ae.get(), &audio_engineLI::track_changed,
            pp.get(), &PlayerPresenterLI::handleTrackChanged);

    // Both the PlayerPresenter and the LyricsManifest need to be aware
    QObject::connect(ae.get(), &audio_engineLI::track_changed,
            lm.get(), [lm, ae] () {
                lm->repopulate_with_lyrics_for_file(ae->current_track().source.toLocalFile());
            }
        );

    QObject::connect(ae.get(), &audio_engineLI::playback_state_changed,
            pp.get(), &PlayerPresenterLI::handlePlaybackStateChanged);

    QObject::connect(ae.get(), &audio_engineLI::seek_finished,
            pp.get(), &PlayerPresenterLI::positionChanged);

    QObject::connect(ae.get(), &audio_engineLI::volume_changed,
            pp.get(), &PlayerPresenterLI::volumeChanged);

    QObject::connect (ae.get(), &audio_engineLI::seek_finished,
            pp.get(), &PlayerPresenterLI::gate_poll_timer);



    QTranslator translator;
    if (translator.load(QLocale::system(), "noname", "_", ":/i18n")) {
        QCoreApplication::installTranslator(&translator);
    }

    // create base engine
    QQmlApplicationEngine engine;

    /* 
        Give the QML engine its own proxy instance allocating a secondary wrapper.
        It will safely invoke 'delete' on this proxy without corrupting the heap
        or interfering with the 'covers' shared_ptr used by the C++ models.

        cover_providerLI is fine here because the engine only care about resolving image://covers...
        and doing this does not break DIP linking in CMakeLists.txt

        QQmlEngine will take ownership of this proxy, and upon program teardown,
        will invoke 'delete' on it.
    */
    engine.addImageProvider("covers", new covers::live::cover_provider(cover_private_storage));

    // load qml module for noname
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app, []() {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.loadFromModule("Player.App", "Main");

    if (engine.rootObjects().isEmpty()) {
        song_factory::teardown();
        ae->teardown();
        return -1;
    }

    const int exitcode = app.exec();
    song_factory::teardown();
    ae->teardown();
    return exitcode;
}
