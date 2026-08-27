#pragma once

#include "abstractmediasequence.hpp"
#include "locallibrary-in.hpp" // interface header
#include "manager-in.hpp"
#include "mediatypes.hpp"

#include <QFuture>
#include <QLoggingCategory>
#include <QUrl>

// Snapshots of directories built from the songs of all known directories and other music sources, if ever supported
// Local Library Logical Database
class LocalLibraryLI : public AbstractMediaSequence, public LocalLibrary
{
    Q_OBJECT
    Q_INTERFACES(LocalLibrary)

public:
    explicit LocalLibraryLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    // Take a snapshot of the songs metadata in given directory path
    QFuture<void> take_snapshot (const QString &dir_path);
    QFuture<void> take_snapshots (const QStringList &paths);
    QFuture<void> retake_all_snapshots ();
    QFuture<void> snapshot_known_directories () override;

    QList<Types::Directory> items () const;
    QStringList paths ();
    Q_INVOKABLE QList<Types::Song> flattened () const override; // equivalent to "All Tracks" 
    Q_INVOKABLE QStringList flattened_sources () const override;

signals:
    void refreshFinished ();

private:
    std::shared_ptr<configuration::manager> cm;
};