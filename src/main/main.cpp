
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTranslator>

#include <algorithm>
#include <iterator>
#include <malloc.h>
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
    // main executable
    mallopt(M_MMAP_THRESHOLD, 128 * 1024);   // big buffers always mmap, so free() returns them to the OS
    mallopt(M_ARENA_MAX, 2);

    QGuiApplication app(argc, argv);

    constexpr char name [] { PROJECT_NAME };
    constexpr QLatin1StringView name_qstr { PROJECT_NAME };

    QCoreApplication::setApplicationName( name_qstr );

    QLoggingCategory::setFilterRules(
        "coverprovider.debug=false"
    );

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

            // preemptt first song with default settings
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
        composition root: instantiate each component separately, in order.
    */

    // boot ae and immediately load and play the first track
    auto ae = std::make_shared<audio_engine_LI>    (nullptr, backend);
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

    // cover cache to hold the shortcuts covers across the entire session
    auto cs = std::make_shared<covers::live::cover_storage>();
    
    auto cm = std::make_shared<managerLI>         (nullptr);
    auto ll = std::make_shared<LocalLibraryLI>    (nullptr, cm);
    auto wi = std::make_shared<WindowGeometryLI>  (nullptr, cm);
    auto pp = std::make_shared<PlayerPresenterLI> (nullptr, cm, ae, pq, wi, cs);
    auto lm = std::make_shared<LyricsManifestLI>  (nullptr, ae); // ae to track position
    auto sl = std::make_shared<ShortcutsListLI>   (nullptr, cm);
    auto pl = std::make_shared<PlaylistsListLI>   (nullptr, cm);
    auto mf = std::make_shared<MediaFilterLI>     ();

    // bind concrete<->concrete signals
    QObject::connect (ae.get(), &audio_engine_LI::track_changed,
            pq.get(), &PlayQueueLI::handle_track_changed);

    QObject::connect(ae.get(), &audio_engine_LI::tape_ran_out,
            pq.get(), &PlayQueueLI::handle_queued_tracks_finished);

    QObject::connect(ae.get(), &audio_engine_LI::track_changed,
            pp.get(), &PlayerPresenterLI::handle_track_changed);

    // both the PlayerPresenter and the LyricsManifest need to be aware of track changes
    QObject::connect(ae.get(), &audio_engine_LI::track_changed,
            lm.get(), &LyricsManifestLI::load_current_track_lyrics);

    QObject::connect(ae.get(), &audio_engine_LI::playback_state_changed,
            pp.get(), &PlayerPresenterLI::playback_state_changed);

    QObject::connect(ae.get(), &audio_engine_LI::seek_finished,
            pp.get(), &PlayerPresenterLI::seek_finished);

    QObject::connect(ae.get(), &audio_engine_LI::volume_changed,
            pp.get(), &PlayerPresenterLI::volume_changed);

    QObject::connect(ae.get(), &audio_engine_LI::relative_seek_before_zero_performed,
            pq.get(), &PlayQueueLI::prev_at_end);

    first_track.then([&pp](const Types::Song &first) {
        qCDebug (l_main) << "Acknowledging the presenter for first track " << first.source;
        if (!first.is_valid()) return;

        // make pp aware of the first track
        pp->handle_track_changed();
    });

    // read user's music directories, and load the music library as a whole
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
    engine.addImageProvider("covers", new covers::live::cover_provider(cs));

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
