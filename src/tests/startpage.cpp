#include "audioengine.hpp"
#include "coverproviderproxy.hpp"
#include "locallibrary.hpp"
#include "mediatypes.hpp"
//#include "serialize.hpp"
#include "serialize.hpp"
#include "shortcutslist.hpp"
#include "songfactory.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>
#include <qabstractitemmodel.h>
#include <qloggingcategory.h>
#include <variant>

Q_LOGGING_CATEGORY(startpagetest, "noname.startpagetest")

int
main (int argc, char ** argv)
{
    // Test how the start page would look like with real user data.
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(R"(
        qt.multimedia*=false
        qt.multimedia*.warning=true
        qt.multimedia*.critical=true
    )");

    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    /*

    auto &conf = configuration::manager::instance();

    // Load configuration for the first time, the file must not be auto created if no write_lines was called
    QStringList known_music_directories_lines = conf.read_lines(configuration::conf_file_type::known_music_directories);

    qCDebug (startpagetest) << "Known music directories paths are:";
    for (const QString &line : known_music_directories_lines) {
        qCDebug (startpagetest) << line;
    }
    qCDebug (startpagetest) << "That's all known music directories.";

    */

    // get access to the shortcuts list that will be displayed

    auto &shortcuts_list = ShortcutsList::instance();

    // cover cache to hold the shortcuts covers
    std::shared_ptr<cover_provider> covers = std::make_shared<cover_provider>();


    QList<QUrl> shortcuts {
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/stars.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/notallowed.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/yummy.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/heykids.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/vanished.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lifeinmono.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/bangbangbangbang.flac")
    };

    song_factory::batch_extract(shortcuts, covers).then(
        &app,
        [&shortcuts_list](const QList<Types::Song> loaded_shortcuts) {
            /*
            QList<Types::Any> any_shortcuts;
            any_shortcuts.reserve(loaded_shortcuts.size());

            // convert Types::Song to Types::Any
            std::copy(loaded_shortcuts.begin(), loaded_shortcuts.end(), std::back_inserter(any_shortcuts));
            */

            shortcuts_list.batch_append(loaded_shortcuts);

            // load a song to pop up the miniplayer
            auto &audioengine = audio_engine::instance();

            Types::Any testsong = shortcuts_list.items().at(1);

            if (std::holds_alternative<Types::Song>(testsong)) {
                audioengine.load(std::get<Types::Song>(testsong));
            }
        }
    );


    /*  load the songs from the known music directories and display as
        a folder-separated view of all available songs */
    auto &ll = LocalLibrary::instance();
    ll.chosen_cover_provider = covers;

    // trigger first refresh
    ll.snapshot_known_directories().then(&app, [&ll]() {
        /*
        qCDebug (startpagetest) << "Indices created. Will print all loaded songs right next.";

        QList<Types::Directory> snapshots = ll.items();

        for (const Types::Directory &dir : snapshots) {
            qCDebug (startpagetest) << debug::serialize(dir);
        }

        */

        // read from the roles here instead...

        
        
        /*
        debug::dump_list_model(
            startpagetest(), 
            ll,
            "LocalLibrary"
        );*/

        /* --- search test

        // Safely retrieve the item
        auto first_item_opt = ll.item_at(0);
        if (!first_item_opt.has_value()) {
            qCWarning(startpagetest) << "LocalLibrary is empty. Cannot perform search test.";
            return;
        }

        // Safely evaluate the active variant type
        Types::Any &first_known_directory_any = first_item_opt.value().get();
        auto *first_known_directory = std::get_if<Types::Directory>(&first_known_directory_any);

        if (!first_known_directory) {
            qCWarning(startpagetest) << "First item is not a Directory.";
            return;
        }

        // Build a test playlist to test search
        PlaylistSequence songs (first_known_directory->songs);

        qCDebug(startpagetest) << "Songs in first directory: " << songs.itemCount();

        // Search 
        QList<QPersistentModelIndex> search_results = songs.search_by_title("обезьянка");

        qCDebug(startpagetest) << "Search results count: " << search_results.size();

        // Print results by querying the correct sequence (songs, not ll)
        for (const QPersistentModelIndex &i : search_results) {
            auto resolved_song = songs.pointed_to(i);
            if (resolved_song.has_value()) {
                debug::print(l_mediasequences(), debug::serialize(resolved_song.value().get()));
            }
        }

    

         --- end search */

        // print flattened ll
        debug::print(startpagetest(), debug::serialize(ll.flattened()));
    });
    
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


    // load qml
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Player.TestQmls", "StartPageTest");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
