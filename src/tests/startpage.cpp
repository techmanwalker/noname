#include "configuration.hpp"
#include "coverproviderproxy.hpp"
// #include "serialize.hpp"
#include "shortcutslist.hpp"
#include "songfactory.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>
#include <qloggingcategory.h>

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

    auto &conf = configuration::manager::instance();

    // Load configuration for the first time, the file must not be auto created if no write_lines was called
    QStringList known_music_directories_lines = conf.read_lines(configuration::conf_file_type::known_music_directories);

    qCInfo(startpagetest) << "Known music directories paths are:";
    for (const QString &line : known_music_directories_lines) {
        qCInfo(startpagetest) << line;
    }
    qCInfo(startpagetest) << "That's all known music directories.";

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

    song_factory::batch_extract(shortcuts, covers).then([&shortcuts_list](const QList<Types::Song> loaded_shortcuts) {
        QList<Types::Any> any_shortcuts;
        any_shortcuts.reserve(loaded_shortcuts.size());

        // convert Types::Song to Types::Any
        std::copy(loaded_shortcuts.begin(), loaded_shortcuts.end(), std::back_inserter(any_shortcuts));

        shortcuts_list.batch_append(any_shortcuts);
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
    const QUrl url = QUrl::fromLocalFile("src/tests/startpagetest.qml");
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
