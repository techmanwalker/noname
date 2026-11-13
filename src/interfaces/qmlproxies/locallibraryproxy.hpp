#pragma once

#include "virtual/locallibrary.hpp"

#include <QIdentityProxyModel>
#include <QtQmlIntegration>

#include "listproxybase.hpp"

class LocalLibraryProxy : public QIdentityProxyModel,
                          public ListProxyBase<LocalLibraryProxy, LocalLibrary>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalLibrary)
    QML_SINGLETON

public:
    explicit LocalLibraryProxy(QObject *parent = nullptr) : QIdentityProxyModel(parent) {
        forward(SIGNAL(refreshFinished()), SIGNAL(refreshFinished()));
    }

    Q_INVOKABLE QFuture<void> snapshot_known_directories() {
        return call_or(QtFuture::makeReadyVoidFuture(), &LocalLibrary::snapshot_known_directories);
    }
    Q_INVOKABLE QList<Types::Song> flattened() const {
        return call(&LocalLibrary::flattened);
    }
    Q_INVOKABLE QStringList flattenedSources() const {
        return call(&LocalLibrary::flattened_sources);
    }

signals:
    void refreshFinished();
};
