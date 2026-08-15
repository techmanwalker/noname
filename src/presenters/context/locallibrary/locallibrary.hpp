#pragma once

#include "abstractmediasequence.hpp"
#include "coverprovider.hpp"
#include "mediatypes.hpp"

#include <QFuture>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QUrl>

// Snapshots of directories built from the songs of all known directories and other music sources, if ever supported
class LocalLibrary : public AbstractMediaSequence
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:

    // disable copy and assignment for single instance
    LocalLibrary(const LocalLibrary&) = delete;
    LocalLibrary &operator=(const LocalLibrary&) = delete;

    static LocalLibrary
    &instance ();

    static LocalLibrary *
    create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Take a snapshot of the songs metadata in given directory path
    QFuture<void> take_snapshot (const QString &dir_path);
    QFuture<void> take_snapshots (const QStringList &paths);
    QFuture<void> retake_all_snapshots ();
    QFuture<void> snapshot_known_directories ();

    QList<Types::Directory> items () const;
    QStringList paths ();
    Q_INVOKABLE QList<Types::Song> flattened () const; // equivalent to "All Tracks" 
    Q_INVOKABLE QStringList flattened_sources () const;

    // where are loaded covers extracted to?
    std::shared_ptr<covers::live::cover_provider> chosen_cover_provider;

signals:
    void refreshFinished ();

private:
    // private constructor to disallow external creations
    explicit LocalLibrary(QObject *parent = nullptr);

};