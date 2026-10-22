
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <algorithm>
#include <iterator>
#include <memory>

#include <CLI/CLI.hpp>

#include "audioengine.hpp"
#include "configuration.hpp"
#include "coverprovider.hpp"
#include "coverstorage.hpp"
#include "dump.hpp"
#include "filter.hpp"
#include "locallibrary.hpp"
#include "lyricsmanifest.hpp"
#include "playerpresenter.hpp"
#include "playlistslist.hpp"
#include "playqueue.hpp"
#include "shortcutslist.hpp"
#include "songfactory.hpp"
#include "soundio/backends.hpp"
#include "windowgeometry.hpp"

#include "locallibraryproxy.hpp"
#include "lyricsmanifestproxy.hpp"
#include "mediafilterproxy.hpp"
#include "playerpresenterproxy.hpp"
#include "playqueueproxy.hpp"
#include "shortcutslistproxy.hpp"
#include "playlistslistproxy.hpp"
#include "windowgeometryproxy.hpp"

#include "backendstrings.hpp"

Q_LOGGING_CATEGORY(l_main, "app")

int
main (int argc, char ** argv)
{
    // real entry point for this player app main executable
    // which must take the project name when I decide what it will be.

    QGuiApplication app(argc, argv);

    constexpr char name [] { PROJECT_NAME };
    constexpr QLatin1StringView name_qstr { PROJECT_NAME };

    QCoreApplication::setApplicationName( name_qstr );

    QLoggingCategory::setFilterRules(
        "coverprovider.debug=false"
    );

    // cover cache to hold the shortcuts covers across the entire session
    std::shared_ptr<covers::live::cover_storage> cover_private_storage = std::make_shared<covers::live::cover_storage>();

    // default target backend for soundio
    audio_backend backend { audio_backend::pulseaudio };

    // song paths listed as command line arguments
    QStringList arg_paths;

    // try to kick in as fast as possible
    QUrl first_track_source;
    QFuture<Types::Song> first_track;

    {
        // short lived

        // cli options
        CLI::App cli { name };

        std::string audio_backend_str { "pulseaudio" };
        using map::audio_backends_list;

        cli.add_option(
            "-b,--audio-backend",
            audio_backend_str,
            complete_help_view.data()
        );

        cli.allow_extras();

        

        using enum audio_backend;

        try {
            cli.parse(argc, argv);
        } catch (const CLI::ParseError &e) {
            return cli.exit(e);
        }

        backend = map::string_to_backend (audio_backend_str);

        // Add to the play queue any path that's readable by song_factory as a track
        if (cli.remaining().size() > 0) {

            // read first song metadata with default settings
            first_track_source = QUrl::fromLocalFile(QString::fromStdString(cli.remaining()[0]));
            first_track = song_factory::extract(
                first_track_source, 
                {}
            );

            // morph std::vector<std::string> -> QStringList
            std::ranges::transform (cli.remaining(), std::back_inserter(arg_paths), &QString::fromStdString);
        }
    }

    /*  
        Composition root: instantiate concrete implementations behind each
        interface boundary. Order reflects the dependency graph (pq needs ae;
        pn needs ae+pq+lm). Ownership is shared_ptr only, to avoid
        double-ownership with Qt's parent/child teardown, hence all initialized
        with nullptr.
    */

    // boot ae and immediately load and play the first track
    auto ae = std::make_shared<audio_engineLI>    (nullptr, backend);
    // do not wait for the metadata to be read
    if (!first_track_source.isEmpty()) {
        ae->load(first_track_source);
        ae->play();
    }

    // load the remaining, the switch should be automagic
    auto pq = std::make_shared<PlayQueueLI>       (nullptr, ae);
    pq->spawn_queue(arg_paths);

    using MediaTransforms::MediaFilterLI;
    using configuration::managerLI;
    
    auto cm = std::make_shared<managerLI>         (nullptr);
    auto ll = std::make_shared<LocalLibraryLI>    (nullptr, cm);
    auto wi = std::make_shared<WindowGeometryLI>  (nullptr, cm);
    auto pp = std::make_shared<PlayerPresenterLI> (nullptr, cm, ae, pq, wi);
    auto lm = std::make_shared<LyricsManifestLI>  (nullptr, ae); // ae to track position
    auto sl = std::make_shared<ShortcutsListLI>   (nullptr, cm);
    auto pl = std::make_shared<PlaylistsListLI>   (nullptr, cm);
    auto mf = std::make_shared<MediaFilterLI>     ();

    first_track.then([&pp](Types::Song first) {
        if (!first.is_valid()) return;

        // if ae failed to play this song, this will fail too
        pp->acknowledge_current_song(first);
    });

    /*  load the songs from the known music directories and display as
        a folder-separated view of all available songs */
    ll->snapshot_known_directories();

    // load shortcuts and user-defined playlists to display
    sl->read_conf_and_load();
    pl->read_conf_and_load().then(&app, [&pl]() {
        debug::dump_list_model(l_main(), *pl, "PlaylistsList", true);
    });

    // invoke the track_changed handlers if needed
    lm->load_current_track_lyrics();

    // initialize QML proxies
    LyricsManifestProxy   ::inject(lm);
    LocalLibraryProxy     ::inject(ll);
    PlayerPresenterProxy  ::inject(pp);
    PlayQueueProxy        ::inject(pq);
    ShortcutsListProxy    ::inject(sl);
    PlaylistsListProxy    ::inject(pl);
    MediaFilterProxy      ::inject(mf);
    WindowGeometryProxy   ::inject(wi);

    // bind signals
    QObject::connect (ae.get(), &audio_engineLI::track_changed,
            pq.get(), &PlayQueueLI::handle_track_changed);

    QObject::connect(ae.get(), &audio_engineLI::tape_ran_out,
            pq.get(), &PlayQueueLI::handle_queued_tracks_finished);

    // Listen to the audio controller; when metadata updates, notify the UI's data.
    // These stay here, not in the proxy: both ae and pn are concrete pointers, and
    // main.cpp is the one place allowed to wire concrete-to-concrete connections.
    QObject::connect(ae.get(), &audio_engineLI::track_changed,
            pp.get(), [&pq, &ae, &pp]() {

                QUrl current_track_source = ae->current_track_source();

                if (current_track_source.isEmpty()) {
                    // nothing playing
                    pp->acknowledge_current_song({});
                    return;
                }

                // get the song struct from the play queue
                auto prolly_queue_has_it = pq->query_metadata_for_source(current_track_source);

                if (!prolly_queue_has_it.has_value()) return;

                // commit metadata to the presenter
                pp->acknowledge_current_song(prolly_queue_has_it.value().get());
            });

    // Both the PlayerPresenter and the LyricsManifest need to be aware
    QObject::connect(ae.get(), &audio_engineLI::track_changed,
            lm.get(), &LyricsManifestLI::load_current_track_lyrics);

    QObject::connect(ae.get(), &audio_engineLI::playback_state_changed,
            pp.get(), &PlayerPresenterLI::handle_playback_state_changed);

    QObject::connect(ae.get(), &audio_engineLI::seek_finished,
            pp.get(), &PlayerPresenterLI::position_changed);

    QObject::connect(ae.get(), &audio_engineLI::seek_finished,
            pp.get(), &PlayerPresenterLI::seek_finished);

    QObject::connect(ae.get(), &audio_engineLI::volume_changed,
            pp.get(), &PlayerPresenterLI::volume_changed);

    QObject::connect (ae.get(), &audio_engineLI::seek_finished,
            pp.get(), &PlayerPresenterLI::gate_poll_timer);

    QObject::connect(pp.get(), &PlayerPresenterLI::position_changed,
            lm.get(), &LyricsManifestLI::poll_highlighted_line_change);



    QTranslator translator;
    if (translator.load(QLocale::system(), name_qstr, "_", ":/i18n")) {
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

    // load the root qml module
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
