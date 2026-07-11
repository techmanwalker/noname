#include "configuration.hpp"
#include "coverprovider.hpp"
#include "serialize.hpp"
#include "directory.hpp"
#include "mediatypes.hpp"

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <memory>

Q_LOGGING_CATEGORY(refreshlisttest, "noname.tests.refreshlist")

int main (int argc, char ** argv)
{
    QCoreApplication app (argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("noname"));

    std::shared_ptr<cover_provider> covers = std::make_shared<cover_provider>();
    auto &conf = configuration::manager::instance();

    QStringList known_music_directories_lines = conf.read_lines(configuration::conf_file_type::known_music_directories);
    QList<Types::Directory> known_dirs;

    qCDebug (refreshlisttest) << "Known music directories paths are:";
    for (const QString &line : std::as_const(known_music_directories_lines)) {
        qCDebug(refreshlisttest) << line;
    }
    qCDebug (refreshlisttest) << "That's all known music directories.";

    qCDebug (refreshlisttest) << "Creating directories logical indices.";

    for (const QString &line : std::as_const(known_music_directories_lines)) {
        known_dirs.emplace_back(line, covers);
    }

    qCDebug (refreshlisttest) << "Indices created.";

    if (known_dirs.isEmpty()) qFatal() << "At least one known directory needed to run this test."
        << "To add a directory to your music collection, append its absolute path to "
        << configuration::file(configuration::conf_file_type::known_music_directories);
    
    for (Types::Directory &dir : known_dirs) {
        qCDebug (refreshlisttest) << "Refreshing cache for: " << dir.path();
        dir.refresh_cache();

        qCDebug (refreshlisttest) << "Song paths in " << dir.path() << ": " << dir.children_paths();

        dir.songs().then([](QList<Types::Song> songs) {
            qCDebug (refreshlisttest) << "Songs loaded: ";

            for (const Types::Song &song : songs) {
                qCDebug (refreshlisttest) << debug::serialize(song);
            }
        });

    }

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