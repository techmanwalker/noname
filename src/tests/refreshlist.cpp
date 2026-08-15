#include "configuration.hpp"
#include "coverprovider.hpp"
#include "coverstorage.hpp"
#include "locallibrary.hpp"
#include "mediatypes.hpp"
#include "serialize.hpp"

#include <QCoreApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(refreshlisttest, "noname.tests.refreshlist")

int main (int argc, char ** argv)
{
    QCoreApplication app (argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    // cover cache to hold the shortcuts covers across the entire session
    std::shared_ptr<covers::live::cover_storage> cover_private_storage = std::make_shared<covers::live::cover_storage>();
    std::shared_ptr<covers::live::cover_provider> covers = std::make_shared<covers::live::cover_provider>(cover_private_storage);

    auto &conf = configuration::manager::instance();

    QStringList known_music_directories_lines = conf.read_lines(configuration::conf_file_type::known_music_directories);

    qCDebug (refreshlisttest) << "Known music directories paths are:";
    for (const QString &line : std::as_const(known_music_directories_lines)) {
        qCDebug(refreshlisttest) << line;
    }
    qCDebug (refreshlisttest) << "That's all known music directories.";

    qCDebug (refreshlisttest) << "Creating logical directory snapshots.";

    // use the dedicated singleton

    auto &ll = LocalLibrary::instance();

    /* automatically reads the config file with the list of directories
       this singleton list will load
    */
    ll.snapshot_known_directories().then([&ll]() {
        qCDebug (refreshlisttest) << "Indices created. Will print all loaded songs right next.";

        QList<Types::Directory> snapshots = ll.items();

        for (const Types::Directory &dir : snapshots) {
            qCDebug (refreshlisttest) << debug::serialize(dir);
        }
    });

    


    

    /*

    QList<QFuture<Types::Song>> songs = dir.songs();
    qCDebug (refreshlisttest) << "dir.songs() did not lock. Hooray.";

    // Use a QTimer to poll the status periodically on the main thread
    QTimer *poll_timer = new QTimer(&app);
    QObject::connect(poll_timer, &QTimer::timeout, [&songs]() {
        qDebug() << "The poll timer did run.";
        
        for (qsizetype i = 0; i < songs.size(); ++i) {
            // Only print if the future is hanging to avoid spamming the console 
            // with 100+ lines every second
            if (!songs.at(i).isFinished()) {
                qDebug() << "Status of future " << i << ": pending.";
            } else {
                qDebug() << "Status of future " << i << ": " << debug::serialize(songs.at(i));
            }
        }
        qDebug() << "--------------------------------------------------";
    });
    
    poll_timer->start(1000);

    */

    return app.exec();
}