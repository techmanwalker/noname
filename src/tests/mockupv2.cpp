#include "coverprovider.hpp"
#include "coverstorage.hpp"
#include "playqueue.hpp"
#include "songfactory.hpp"
// #include "serialize.hpp"

#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QString>
#include <qloggingcategory.h>
//#include <QDirIterator>

Q_LOGGING_CATEGORY(mockupv2, "noname.tests.mockupv2")

using namespace std::chrono_literals; // for _s suffix

int
main (int argc, char ** argv)
{

    // Currently, just a GUI test app
    // create base application
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    // auto &conf = configuration::manager::instance();

    // Load configuration for the first time, the file must not be auto created if no write_lines was called
    /* WORKS FINE

    qCDebug (mockupv2) << "Known music directories paths are:";
    for (const QString &line : conf.read_lines(configuration::conf_file_type::known_music_directories)) {
        qCDebug (mockupv2) << line;
    }
    qCDebug (mockupv2) << "That's all known music directories.";

    qCDebug (mockupv2) << "Write a single line test...";
    auto write_future = conf.write_lines(configuration::conf_file_type::known_music_directories, QStringList {
        "/home/notangel/Música"
    });
    write_future.then([&conf](bool did_write_finish_successfully) {
        Q_UNUSED(did_write_finish_successfully)

        qCDebug (mockupv2) << "Known music directories paths after writing are:";

        for (const QString &line : conf.read_lines(configuration::conf_file_type::known_music_directories)) {
            qCDebug (mockupv2) << line;
        }
        qCDebug (mockupv2) << "That's all known music directories.";
    });
    */

    /*
    
    // Populate it (example - load from file or hardcode for testing)
    testLyrics.appendLyric(0, "You shut your mouth");
    testLyrics.appendLyric(5000, "How can you say mockupvI go about things the wrong way?");
    testLyrics.appendLyric(10000, "I am human and I need to be loved");
    testLyrics.appendLyric(15000, "Just like everybody else does");

    */

    // Cover cache
    std::shared_ptr<covers::live::cover_storage> cover_private_storage = std::make_shared<covers::live::cover_storage>();

    std::shared_ptr<covers::live::cover_provider> covers = std::make_shared<covers::live::cover_provider>(cover_private_storage);

    // Playlist model
    auto &nextQueue = PlayQueue::instance();
    nextQueue.chosen_cover_provider = covers;

    // Populate it (example)
    // Now with concurrent batch loading: it will extract metadata in parallel 
    // and safely perform a single massive insertion on the model once ready.

    QList<QUrl> new_queue {
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lilith.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/apotionforlove.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/allthethingsshesaid.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/bittersweetsymphony.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/eric.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/lifeinmono.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/onlytime.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/showmehow.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/runaway.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/telephones.flac"),
        QUrl::fromLocalFile("/home/notangel/Documentos/Archivos del teléfono/Music/ykwim.flac")
    };
    
    song_factory::batch_extract(new_queue, {}).then(&app, [&nextQueue, covers](const QList<Types::Song> loaded_songs) {
        /*
        QList<Types::Any> any_songs(loaded_songs.begin(), loaded_songs.end());
        qDebug().noquote() << QJsonDocument(debug::serialize(any_songs)).toJson(QJsonDocument::Indented);
        */
        for (const Types::Song &song : loaded_songs) {
            covers->register_cover_reference(song.cover);
        }

        // when the playlist finishes loading, replace the current PlayQueue songs with these ones
        nextQueue.respawn_queue(std::move(loaded_songs));
    });


    // After the queue respawn, the playback controller will switch to the first song :D

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
    /*  Give the QML engine its own proxy instance allocated with 'new'.
        It will safely invoke 'delete' on this proxy without corrupting the heap
        or interfering with the 'covers' shared_ptr used by the C++ models.
    */
    engine.addImageProvider("covers", new covers::live::cover_provider(cover_private_storage));



    /*
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
    */

    // load qml
    const QUrl url = QUrl::fromLocalFile("src/tests/MockupV2.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    // wait 5 seconds and print nextQueue contents

    /*

    (void) QtConcurrent::run([&nextQueue]() {
        QThread::msleep(5000);

        QList<Types::Song> nt_items(nextQueue.items());
        QList<Types::Any> nt_any (nt_items.begin(), nt_items.end());

        // print
        qDebug().noquote() << QJsonDocument(debug::serialize(nt_any)).toJson();
    });
    */

    return app.exec();
}